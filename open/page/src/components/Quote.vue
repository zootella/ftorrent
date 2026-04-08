<script setup>

/*
Between the fall of the Berlin Wall and the burst of the dot-com bubble, something remarkable happened: politicians, technologists, diplomats, dissidents, and engineers briefly shared a common vision. They believed that connection — between people, between nations, between ideas — would make the world gentler, more open, more free. They weren't naive. Many of them had lived through wars, dictatorships, and the threat of nuclear annihilation. They simply looked at what was emerging and allowed themselves to hope.

These are their words.
*/
const quotes = [
{
text: 'The world is now enmeshed in webs of telecommunication networks consisting of millions of tiny threads, or capillaries, that not only transmit information of all kinds at lightning speed, but also convey integrated models of social, political and economic behavior.',
author: 'Václav Havel',
url: 'https://en.wikipedia.org/wiki/V%C3%A1clav_Havel',
context: 'Harvard commencement, 1995',
},
{
text: 'The central political task of the final years of this century is the creation of a new model of coexistence among the various cultures, peoples, races and religious spheres within a single interconnected civilization.',
author: 'Václav Havel',
url: 'https://en.wikipedia.org/wiki/V%C3%A1clav_Havel',
context: 'Liberty Medal acceptance, 1994',
},
{
text: 'Our conscience must catch up to our reason, otherwise we are lost.',
author: 'Václav Havel',
url: 'https://en.wikipedia.org/wiki/V%C3%A1clav_Havel',
context: 'Harvard commencement, 1995',
},
{
text: 'The web is more a social creation than a technical one. I designed it for a social effect — to help people work together — and not as a technical toy.',
author: 'Tim Berners-Lee',
url: 'https://en.wikipedia.org/wiki/Tim_Berners-Lee',
context: 'Weaving the Web, 1999',
},
{
text: `Allowing everybody to write their own web page and express some vision of what is true and right — whether that\u2019s a car accident yesterday or how the universe is run — gives us this decentralized, distributed quest for truth, harmony, and understanding.`,
author: 'Tim Berners-Lee',
url: 'https://en.wikipedia.org/wiki/Tim_Berners-Lee',
context: 'UU World interview, 1999',
},
{
text: 'These networks of distributed intelligence will allow us to share information, to connect, and to communicate as a global community. From these connections we will derive robust and sustainable economic progress, strong democracies, better solutions to global and local environmental challenges, improved health care, and — ultimately — a greater sense of shared stewardship of our small planet.',
author: 'Al Gore',
url: 'https://en.wikipedia.org/wiki/Al_Gore',
context: 'Address to the ITU, Buenos Aires, 1994',
},
{
text: 'I see a new Athenian Age of democracy forged in the fora the GII will create.',
author: 'Al Gore',
url: 'https://en.wikipedia.org/wiki/Al_Gore',
context: 'Address to the ITU, Buenos Aires, 1994',
},
{
text: 'We will create a civilization of the Mind in Cyberspace. May it be more humane and fair than the world your governments have made before.',
author: 'John Perry Barlow',
url: 'https://en.wikipedia.org/wiki/John_Perry_Barlow',
context: 'A Declaration of the Independence of Cyberspace, 1996',
},
{
text: 'More than ever before in human history, we share a common destiny. We can master it only if we face it together.',
author: 'Kofi Annan',
url: 'https://en.wikipedia.org/wiki/Kofi_Annan',
context: 'United Nations, late 1990s',
},
{
text: 'It has been said that arguing against globalization is like arguing against the laws of gravity.',
author: 'Kofi Annan',
url: 'https://en.wikipedia.org/wiki/Kofi_Annan',
context: 'Millennium Report, 2000',
},
{
text: 'The United Nations itself was created in the belief that dialogue can triumph over discord, that diversity is a universal virtue, and that the peoples of the world are far more united by their common fate than they are divided by their separate identities.',
author: 'Kofi Annan',
url: 'https://en.wikipedia.org/wiki/Kofi_Annan',
context: 'Yale University, 2002',
},
{
text: 'The future of the Net is connected to the future of community, democracy, education, science, and intellectual life — some of the human institutions people hold most dear.',
author: 'Howard Rheingold',
url: 'https://en.wikipedia.org/wiki/Howard_Rheingold',
context: 'The Virtual Community, 1993',
},
{
text: 'The Net offers us a chance to take charge of our own lives and to redefine our role as citizens of local communities and of a global society.',
author: 'Esther Dyson',
url: 'https://en.wikipedia.org/wiki/Esther_Dyson',
context: 'Release 2.0, 1997',
},
{
text: 'The Net will celebrate human nature and human diversity if we do it right.',
author: 'Esther Dyson',
url: 'https://en.wikipedia.org/wiki/Esther_Dyson',
context: 'Release 2.0, 1997',
},
{
text: 'Globalization genuinely means interdependence, which means none of us who are fortunate can any longer help ourselves unless we are prepared to help our neighbors.',
author: 'Bill Clinton',
url: 'https://en.wikipedia.org/wiki/Bill_Clinton',
context: 'World Economic Forum, Davos, 2000',
},
{
text: 'Being digital is positive. It can flatten organizations, globalize society, decentralize control, and help harmonize people.',
author: 'Nicholas Negroponte',
url: 'https://en.wikipedia.org/wiki/Nicholas_Negroponte',
context: 'Being Digital, 1995',
},
{
text: 'Of all the endeavors we humans are now engaged in, perhaps the grandest of them all is the steady weaving together of our lives, minds, and artifacts into a global-scale network.',
author: 'Kevin Kelly',
url: 'https://en.wikipedia.org/wiki/Kevin_Kelly_(editor)',
context: 'New Rules for the New Economy, 1998',
},
{
text: 'The only factor becoming scarce in a world of abundance is human attention.',
author: 'Kevin Kelly',
url: 'https://en.wikipedia.org/wiki/Kevin_Kelly_(editor)',
context: 'New Rules for the New Economy, 1998',
},
{
text: 'We reject kings, presidents and voting. We believe in rough consensus and running code.',
author: 'David Clark',
url: 'https://en.wikipedia.org/wiki/David_D._Clark',
context: 'IETF, 1992',
},
{
text: 'In Cyberspace, the First Amendment is a local ordinance.',
author: 'Mitch Kapor',
url: 'https://en.wikipedia.org/wiki/Mitch_Kapor',
context: `Foreword to The Big Dummy\u2019s Guide to the Internet, 1993`,
},
{
text: 'In the Cold War, the most frequently asked question was \u201CHow big is your missile?\u201D In globalization, the most frequently asked question is \u201CHow fast is your modem?\u201D',
author: 'Thomas Friedman',
url: 'https://en.wikipedia.org/wiki/Thomas_Friedman',
context: 'The Lexus and the Olive Tree, 1999',
},
{
text: 'The Net interprets censorship as damage and routes around it.',
author: 'John Gilmore',
url: 'https://en.wikipedia.org/wiki/John_Gilmore_(activist)',
context: 'TIME magazine, 1993',
},
{
text: 'The information superhighway is more than a shortcut to every book in the Library of Congress. It is creating a totally new, global social fabric.',
author: 'Nicholas Negroponte',
url: 'https://en.wikipedia.org/wiki/Nicholas_Negroponte',
context: 'Being Digital, 1995',
},
]

// Pick quote by UTC day number
const day = Math.floor(Date.now() / 86_400_000)
const quote = quotes[day % quotes.length]
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
	max-width: 40rem;
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
