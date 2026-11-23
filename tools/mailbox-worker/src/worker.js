// TamaPoke mailbox for friends far away: the same small API as ntfy.sh, so the
// board firmware works with either (MAILBOX_URL in config.h / config_private.h).
//
//   POST /<box>                         body: text (up to 4 KB) -> {"id","time","event":"message","topic"}
//   GET  /<box>/json?poll=1&since=<t>   one JSON line per message with time >= t (unix seconds), or since=all
//
// Boxes are random names that each board tells its friends when they meet; the
// boards encrypt and sign every message with their pairing key, so the mailbox
// only ever stores unreadable text. A message the board has read (it now polls
// with a later "since") is deleted once it is a day old; one never read expires
// after 30 days. If the KEY secret
// is set, every request needs the header "X-Key: <KEY>" (keeps strangers from
// using your Worker).
const BOX = /^[A-Za-z0-9_-]{8,40}$/;
const TTL = 30 * 86400;
const KEEP_READ = 86400;  // a read message (time < since) stays this long after it was sent
const DEL_MAX = 300;      // deletions per poll (KV allows 1000 operations per request); the rest next time
// Messages per reply, oldest first (keys sort by time). A board reads ~16 KB of a reply (~85 lines) and
// asks for the rest from its new cursor; with no cap a big backlog went over the KV limit (an error,
// and a box stuck for good). Lists + gets + deletes stay well under 1000.
const GET_MAX = 80;

export default {
  async fetch(req, env, ctx) {
    if (env.KEY && req.headers.get('X-Key') !== env.KEY) return new Response('forbidden\n', { status: 403 });
    const url = new URL(req.url);
    const parts = url.pathname.split('/').filter(Boolean);
    const box = parts[0];
    if (!box || !BOX.test(box)) return new Response('not found\n', { status: 404 });

    if (req.method === 'POST' && parts.length === 1) {
      const body = await req.text();
      if (!body || body.length > 4096) return new Response('bad body\n', { status: 400 });
      const time = Math.floor(Date.now() / 1000);
      const id = String(time).padStart(10, '0') + '-' + crypto.randomUUID().slice(0, 8);
      await env.MAIL.put(`${box}/${id}`, body, { expirationTtl: TTL, metadata: { time } });
      return Response.json({ id, time, event: 'message', topic: box });
    }

    if (req.method === 'GET' && parts.length === 2 && parts[1] === 'json') {
      const s = url.searchParams.get('since') || 'all';
      const since = s === 'all' ? 0 : parseInt(s, 10) || 0;
      const now = Math.floor(Date.now() / 1000);
      // The keys first, then all the messages at once. Read one by one, a box with ~80 messages
      // took 9.5 s, longer than a board waits (8 s), so its cursor never moved and the box only grew.
      const keys = [];
      const read = [];  // already taken by the board (older than its "since") and a day old: delete
      let cursor;
      do {
        const page = await env.MAIL.list({ prefix: `${box}/`, cursor });
        for (const k of page.keys) {
          const id = k.name.slice(box.length + 1);
          const time = (k.metadata && k.metadata.time) || parseInt(id, 10) || 0;
          if (time >= since) {
            if (keys.length < GET_MAX) keys.push({ name: k.name, id, time });
          } else if (since <= now + 60 && time < now - KEEP_READ && read.length < DEL_MAX) read.push(k.name);
        }
        // read keys sort before the unread ones: once the reply is full, the rest can wait
        cursor = page.list_complete || keys.length >= GET_MAX ? undefined : page.cursor;
      } while (cursor);
      const bodies = await Promise.all(keys.map((k) => env.MAIL.get(k.name)));
      let out = '';
      keys.forEach((k, i) => {
        if (bodies[i] === null) return;  // expired between list and get
        out += JSON.stringify({ id: k.id, time: k.time, event: 'message', topic: box, message: bodies[i] }) + '\n';
      });
      // after the reply, so a poll is not slower for it
      if (read.length) ctx.waitUntil(Promise.all(read.map((n) => env.MAIL.delete(n))));
      return new Response(out, { headers: { 'content-type': 'application/x-ndjson' } });
    }

    return new Response('not found\n', { status: 404 });
  },
};
