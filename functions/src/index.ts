import * as functions from 'firebase-functions';
import * as admin from 'firebase-admin';

admin.initializeApp();

const QUEUE_PATH = 'queues/{queueId}';

interface LinesData { [line: string]: number; }

function computeRecommended(lines: LinesData): { recommendedLine: number | null; total: number } {
  let best: number | null = null;
  let bestCount = Number.MAX_SAFE_INTEGER;
  let total = 0;
  for (const [k, vRaw] of Object.entries(lines)) {
    const v = typeof vRaw === 'number' ? vRaw : parseInt(String(vRaw)) || 0;
    total += v;
    const lineNum = parseInt(k, 10);
    if (isNaN(lineNum)) continue;
    if (v < bestCount || (v === bestCount && (best === null || lineNum < best))) {
      best = lineNum;
      bestCount = v;
    }
  }
  return { recommendedLine: best, total };
}

// Trigger when a specific line count is updated: /queues/{queueId}/lines/{lineNumber}
export const onLineCountWrite = functions.database
  .ref(`${QUEUE_PATH}/lines/{lineNumber}`)
  .onWrite(async (change, context) => {
    const queueRef = change.after.ref.parent?.parent; // points to /queues/{queueId}
    if (!queueRef) return null;

    const snap = await queueRef.child('lines').get();
    const lines = (snap.val() as LinesData) || {};
    const { recommendedLine, total } = computeRecommended(lines);

    await queueRef.update({
      recommendedLine,
      length: total,
      updatedAt: admin.database.ServerValue.TIMESTAMP,
    });
    return null;
  });

// Callable to enqueue automatically on least-populated line
export const enqueueAuto = functions.https.onCall(async (data, context) => {
  const queueId: string = data.queueId || 'queueA';
  const queueRef = admin.database().ref(`queues/${queueId}`);
  const linesRef = queueRef.child('lines');

  const result = await linesRef.transaction((current) => {
    const lines: LinesData = current || { '1': 0 };
    // Compute best line now
    const { recommendedLine } = computeRecommended(lines);
    if (recommendedLine == null) {
      lines['1'] = 1;
    } else {
      const key = String(recommendedLine);
      lines[key] = (lines[key] || 0) + 1;
    }
    return lines;
  });

  if (!result.committed) {
    throw new functions.https.HttpsError('aborted', 'Transaction not committed');
  }

  // Recompute after commit
  const lines = result.snapshot.val() as LinesData;
  const { recommendedLine, total } = computeRecommended(lines);
  await queueRef.update({
    recommendedLine,
    length: total,
    updatedAt: admin.database.ServerValue.TIMESTAMP,
  });
  return { recommendedLine, total, lines };
});

// Callable to enqueue on a specific line
export const enqueueOnLine = functions.https.onCall(async (data, context) => {
  const queueId: string = data.queueId || 'queueA';
  const line: number = parseInt(String(data.line));
  if (isNaN(line) || line <= 0) {
    throw new functions.https.HttpsError('invalid-argument', 'line must be positive integer');
  }

  const queueRef = admin.database().ref(`queues/${queueId}`);
  const linesRef = queueRef.child('lines');

  const result = await linesRef.transaction((current) => {
    const lines: LinesData = current || {};
    const key = String(line);
    lines[key] = (lines[key] || 0) + 1;
    return lines;
  });

  if (!result.committed) {
    throw new functions.https.HttpsError('aborted', 'Transaction not committed');
  }

  // Recompute
  const lines = result.snapshot.val() as LinesData;
  const { recommendedLine, total } = computeRecommended(lines);
  await queueRef.update({
    recommendedLine,
    length: total,
    updatedAt: admin.database.ServerValue.TIMESTAMP,
  });
  return { recommendedLine, total, lines };
});
