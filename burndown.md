# Burn-down

A burn-down is how the user and Claude work through a list of related items, one at a time, across many turns, without rushing any item past the gate or letting any fall through the cracks. It's one part of a session, not a whole session, and it's a core flow of how we work together.

## Where a burn-down comes from

It usually follows a review. The user names a large area and asks for a lengthy turn to look at it, saying what the work is about and giving full context. Claude takes the time to read it all and answers at length, with a numbered list of issues: the bugs, gaps, simplifications, and drifted docs. The user reads that carefully, which takes real time of their own, and replies item by item using the same numbers, deciding some, questioning others, and setting others aside as known and accepted.

At that point a lot is in the air at once. The items are related, all in one topic, and could all be worked at the same time. That's exactly what we don't do. They wait in line, and the burn-down is how they get through it.

## The rules

**The numbers stay.** The review's numbering is the shared vocabulary for the rest of the burn-down. An item keeps its number to the end, and an item found along the way gets the next unused one.

**One item at a time.** We work exactly one item until it's resolved. Not in numerical order: Claude chooses which comes next, and the user may override that choice.

**An item gets its full introduction when its turn comes.** The review gave each item a short space, because it had to fit a dozen beside it. When the burn-down reaches an item, Claude introduces it again, from the beginning and at full length, as if the user had never read the review: what the code does there and when, what goes wrong and how a user would meet it, how likely that is and how sure Claude is, what changes and what it costs, the options, and what Claude recommends. The point of taking items one at a time is that each one gets the room it needs, so it gets it.

**Options are named by what they do.** When an item has choices, each is described in plain words every time it's mentioned. No letters, numbers, or nicknames for options: across several turns they pile up and stop meaning anything, and the item's own number is the only label the burn-down needs.

**An item takes as many turns as it needs.** Research, a question, an answer, a change, a test on a real machine, another question. Nothing hurries it.

**Only the user resolves an item.** An item is resolved when the user says it is: "that one's resolved," "ok, onwards," "done, next." Nothing else resolves it. A change being made doesn't, Claude's recommendation doesn't, and neither does the user asking about something else.

**An open question is repeated until it's answered.** If Claude asked something and the reply doesn't answer it, the question comes back in the next turn, and the one after, until the user answers it. The user saying "pick the next one" while a question is open doesn't close it either. Claude asks the question again rather than reading the reply as an answer.

**Resolved means gone.** A resolved item never appears again: no summary chart turning red to green, no table of what's done, no recap of what's finished. A list of what's left, if one is ever shown, holds only what's left.

**Every response is about one item.** A turn opens with the current item's number and name, and it stays there. It doesn't scope out to the whole list, survey what's coming, or mention items that are waiting. If work on one item turns up something for another, or a new issue, Claude notes it in a line and it joins the line; it isn't worked now.

**Claude runs the process.** The user doesn't track what's next or what's left. When the user resolves an item, Claude takes up the next one in that same turn: names it and starts on it. Claude never moves on before the user says the item is resolved, which rushes the gate. And Claude never leaves the next step unspoken, with everyone standing there and nobody stepping forward. Every turn ends with exactly what the item needs from the user: a decision, a test to run, or "is this one resolved?"

**It ends when nothing remains.** When the last item is resolved, Claude says so in a sentence, and the burn-down is over. Commits wait until then: the user reviews and commits once, at the end, so no commit message is proposed while items remain.

## Why it works

The user's attention is the scarce thing. A review turn spends it once, deliberately, on the whole picture. After that, one item per stretch of turns keeps each decision small enough to make well, keeps each change small enough to review, and means nothing slips past for want of being looked at. Claude holding the line, the order, and the open questions is what lets the user spend that attention on the decisions themselves.
