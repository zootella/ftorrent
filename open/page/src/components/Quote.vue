<script setup>

/*
Between the fall of the Berlin Wall and the burst of the dot-com bubble, something remarkable happened: politicians, technologists, diplomats, dissidents, and engineers briefly shared a common vision. They believed that connection — between people, between nations, between ideas — would make the world gentler, more open, more free. They weren't naive. Many of them had lived through wars, dictatorships, and the threat of nuclear annihilation. They simply looked at what was emerging and allowed themselves to hope.

These are their words.
*/
const quoteBlock = `
The world is now enmeshed in webs of telecommunication networks consisting of millions of tiny threads, or capillaries, that not only transmit information of all kinds at lightning speed, but also convey integrated models of social, political and economic behavior.
Václav Havel
https://en.wikipedia.org/wiki/V%C3%A1clav_Havel
Harvard commencement, 1995
0.07755774404520666

The central political task of the final years of this century is the creation of a new model of coexistence among the various cultures, peoples, races and religious spheres within a single interconnected civilization.
Václav Havel
https://en.wikipedia.org/wiki/V%C3%A1clav_Havel
Liberty Medal acceptance, 1994
0.38664069972103354

Our conscience must catch up to our reason, otherwise we are lost.
Václav Havel
https://en.wikipedia.org/wiki/V%C3%A1clav_Havel
Harvard commencement, 1995
0.23649071834335988

The web is more a social creation than a technical one. I designed it for a social effect--to help people work together--and not as a technical toy.
Tim Berners-Lee
https://en.wikipedia.org/wiki/Tim_Berners-Lee
Weaving the Web, 1999
0.31146306296573845

Allowing everybody to write their own web page and express some vision of what is true and right--whether that's a car accident yesterday or how the universe is run--gives us this decentralized, distributed quest for truth, harmony, and understanding.
Tim Berners-Lee
https://en.wikipedia.org/wiki/Tim_Berners-Lee
UU World interview, 1999
0.2526188131845155

These networks of distributed intelligence will allow us to share information, to connect, and to communicate as a global community. From these connections we will derive robust and sustainable economic progress, strong democracies, better solutions to global and local environmental challenges, improved health care, and--ultimately--a greater sense of shared stewardship of our small planet.
Al Gore
https://en.wikipedia.org/wiki/Al_Gore
Address to the ITU, Buenos Aires, 1994
0.8285210226568394

I see a new Athenian Age of democracy forged in the fora the GII [Global Information Infrastructure] will create.
Al Gore
https://en.wikipedia.org/wiki/Al_Gore
Address to the ITU, Buenos Aires, 1994
0.10342464173328692

We will create a civilization of the Mind in Cyberspace. May it be more humane and fair than the world your governments have made before.
John Perry Barlow
https://en.wikipedia.org/wiki/John_Perry_Barlow
A Declaration of the Independence of Cyberspace, 1996
0.5263669552304305

More than ever before in human history, we share a common destiny. We can master it only if we face it together.
Kofi Annan
https://en.wikipedia.org/wiki/Kofi_Annan
United Nations, late 1990s
0.11250984313948409

It has been said that arguing against globalization is like arguing against the laws of gravity.
Kofi Annan
https://en.wikipedia.org/wiki/Kofi_Annan
Millennium Report, 2000
0.13995068252197185

The United Nations itself was created in the belief that dialogue can triumph over discord, that diversity is a universal virtue, and that the peoples of the world are far more united by their common fate than they are divided by their separate identities.
Kofi Annan
https://en.wikipedia.org/wiki/Kofi_Annan
Yale University, 2002
0.5803378271132534

The future of the Net is connected to the future of community, democracy, education, science, and intellectual life--some of the human institutions people hold most dear.
Howard Rheingold
https://en.wikipedia.org/wiki/Howard_Rheingold
The Virtual Community, 1993
0.20985240244912629

The Net offers us a chance to take charge of our own lives and to redefine our role as citizens of local communities and of a global society.
Esther Dyson
https://en.wikipedia.org/wiki/Esther_Dyson
Release 2.0, 1997
0.11347622113022482

The Net will celebrate human nature and human diversity if we do it right.
Esther Dyson
https://en.wikipedia.org/wiki/Esther_Dyson
Release 2.0, 1997
0.8202847368486893

Globalization genuinely means interdependence, which means none of us who are fortunate can any longer help ourselves unless we are prepared to help our neighbors.
Bill Clinton
https://en.wikipedia.org/wiki/Bill_Clinton
World Economic Forum, Davos, 2000
0.3830936191936488

Being digital is positive. It can flatten organizations, globalize society, decentralize control, and help harmonize people.
Nicholas Negroponte
https://en.wikipedia.org/wiki/Nicholas_Negroponte
Being Digital, 1995
0.2031866382584543

Of all the endeavors we humans are now engaged in, perhaps the grandest of them all is the steady weaving together of our lives, minds, and artifacts into a global-scale network.
Kevin Kelly
https://en.wikipedia.org/wiki/Kevin_Kelly_(editor)
New Rules for the New Economy, 1998
0.6950756599654494

The only factor becoming scarce in a world of abundance is human attention.
Kevin Kelly
https://en.wikipedia.org/wiki/Kevin_Kelly_(editor)
New Rules for the New Economy, 1998
0.21394929518333372

We reject kings, presidents and voting. We believe in rough consensus and running code.
David Clark
https://en.wikipedia.org/wiki/David_D._Clark
IETF, 1992
0.15275578306188475

In Cyberspace, the First Amendment is a local ordinance.
Mitch Kapor
https://en.wikipedia.org/wiki/Mitch_Kapor
Foreword to The Big Dummy's Guide to the Internet, 1993
0.5961122632485145

In the Cold War, the most frequently asked question was "How big is your missile?" In globalization, the most frequently asked question is "How fast is your modem?"
Thomas Friedman
https://en.wikipedia.org/wiki/Thomas_Friedman
The Lexus and the Olive Tree, 1999
0.7582365716093247

The Net interprets censorship as damage and routes around it.
John Gilmore
https://en.wikipedia.org/wiki/John_Gilmore_(activist)
TIME magazine, 1993
0.12329638286653044

The information superhighway is more than a shortcut to every book in the Library of Congress. It is creating a totally new, global social fabric.
Nicholas Negroponte
https://en.wikipedia.org/wiki/Nicholas_Negroponte
Being Digital, 1995
0.35994851644267223
`

function makeRandom() {//if you add more quotes, here's how to make new random sorting orders for them
	let s = '\n'
	for (let i = 0; i < 50; i++) {
		s += `${Math.random()}\n`
	}
	console.log(s)
}

function formatQuote(s) {
	return s
		.replace(/\s*--\s*/g, ' \u2014 ') // -- → em-dash, space-padded so the monospaced glyph isn't crowded
		.replace(/(^|[\s\u2014(\[{])"/g, '$1\u2018') // opening " → curly open single (demoted so it nests inside the outer double quotes)
		.replace(/"/g, '\u2019') // remaining " → curly close single
		.replace(/(^|[\s\u2014(\[{])'/g, '$1\u2018') // opening ' → curly open single
		.replace(/'/g, '\u2019') // remaining ' → curly close single, also the apostrophe glyph
}
// The opener set above (start-of-string, whitespace, em-dash, ( [ { ) is
// what flags a " or ' as an opening quote rather than a closing one. When
// adding quotes, eyeball the render; if you see two close-style glyphs
// ('' or '') where an open/close pair belongs, the character before the
// quote isn't in the opener set. Likely offenders:
//   - ellipsis with no trailing space:       ...'yes'
//   - colon or semicolon with no space:      said:"yes"
//   - triple-nested quotation                (we only have two levels of glyphs)
// Easiest fix: add a space in the source. Otherwise widen the opener set
// to include the offending character.

const quoteList = quoteBlock
	.trim()
	.split(/\n\s*\n/)
	.map(block => {
		const lines = block.split('\n').map(l => l.trim())
		return {
			text: formatQuote(lines[0]),
			author: lines[1],
			url: lines[2],
			context: formatQuote(lines[3]),
			order: parseFloat(lines[4]),  // scatters same-author runs across the calendar without scattering them in the source
		}
	})
	.sort((a, b) => a.order - b.order)
const day = Math.floor(Date.now() / 86_400_000)// Pick quote by UTC day number
const quote = quoteList[day % quoteList.length]

</script>
<template>

<div class="quote">
	<div class="quote-title">Quote of the Day</div>
	<blockquote>&ldquo;{{ quote.text }}&rdquo;</blockquote>
	<div class="attribution">&mdash; <a :href="quote.url">{{ quote.author }}</a>, {{ quote.context }}</div>
</div>

</template>
<style scoped>

.quote {
	max-width: 50rem;
	margin: 2rem auto;
	padding: 0 2rem;
	box-sizing: border-box;
	text-align: center;
}

.quote-title {
	font-family: 'Jura', sans-serif;
	font-weight: var(--muted-weight);
	font-size: 1rem;
	color: var(--text-muted);
	margin-bottom: 1.0rem;
}

blockquote {
	font-family: 'Monaspace Radon', sans-serif;
	font-size: 1.3rem;
	font-weight: 300;
	line-height: 1.4;
	color: var(--text);
}

.attribution {
	font-family: 'Jura', sans-serif;
	font-weight: var(--muted-weight);
	font-size: 1rem;
	color: var(--text-muted);
	margin-top: 1.0rem;
}

.attribution a {
	color: inherit;
	text-decoration: underline;
}

</style>
