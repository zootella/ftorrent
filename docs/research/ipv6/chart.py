#!/usr/bin/env python3
import json
import sys
from pathlib import Path

# This is the third of three scripts. It reads data/table.json, which table.py wrote, and draws the report's figures as SVG files in the charts folder beside it. Run it with no arguments: python3 chart.py. There is no plotting library: each figure is a few hundred lines of SVG written by hand from a handful of small helpers below, which keeps the reader's installation at nothing and keeps every pixel under the report's control.
#
# The figures share one look, which is the docs site's look: the brand orange and grays with no hue in them. Orange is the v6 address, in the mosaic's v6 cells and in every line that counts v6 addresses; gray is everything that is not. Dashed is the open device, as stripes across the mosaic's open column and as the dash on every line that counts open devices; solid is everything that is not. So the one cell the report is about, open devices with v6 addresses, is the cell of orange stripes and the dashed orange line, the same in every figure. The labels say v6 address and v4 address, the report's short forms for IPv6 and IPv4, which Section 2.3 of the report defines. The figures also lean on direct labels, legends, and hover text, never on color alone, and every number drawn here is also in the report's tables and in data/table.csv.
#
# Every SVG is written to be dropped inline into a VitePress page or opened on its own. Its colors are CSS variables with fallbacks: on the docs site the site's own text and divider colors take over, so the figures follow its light and dark modes; as a standalone file the fallbacks are the light values. The opening tag sits alone on its first line and the file has no blank lines, which is what lets a markdown include place it as one HTML block.

folder = Path(__file__).resolve().parent/'charts'#where the figures go
table = {}#data/table.json, loaded in main
font = "var(--vp-font-family-base, system-ui, -apple-system, 'Segoe UI', sans-serif)"
orange = '#FF7900'#Pantone 151 C, ftorrent's brand color, the fallback when a figure is opened on its own; inline on the docs site the brand variable takes over. The one cell the report is about, open devices with v6 addresses, wherever it appears. The only hue in the figures; everything else is gray, solid or dashed
width = 720#every figure is this wide in its own units and scales to the page
size_text, size_number = 14, 36#the two type sizes in every figure, in the figure's own units, which scale with it. On the docs site the 720 units span about 690 pixels, so 14 lands at the size the site sets captions and blockquotes, and matches the 13 of the diagrams in the desktop architecture essay, which are drawn 680 wide. Every word and small number is the text size; the mosaic's cell percentages are the number size. Hierarchy comes from ink and weight, not size: a title or heading is bold in the primary ink, a secondary line regular in the secondary ink
line_height = 18#between the baselines of wrapped lines, 1.3 of the text size, which is the spacing the docs site gives a figure's caption, so a footnote inside a figure and the caption under it read as one kind of text
per_char, per_char_bold = 0.54, 0.60#em per character at regular and bold weight, measured on Verdana, the docs site's prose face and the widest face the page sets; the system fallbacks and Inter are narrower, so a layout that fits Verdana fits them. Labels are placed by these estimates, since SVG text has no measure of its own. Bold is set only at the text size, never at the number size, where Verdana's bold reads heavy
# One style per quantity, so the same quantity looks the same in every figure. Orange is the v6 address and gray is not; dashed is the open device and solid is not. The two combine: a dashed orange line is open devices with v6 addresses, the one cell the report is about, and the mosaic says the same with orange and gray stripes for its open column against solid orange and gray for its closed one. Complements are not drawn as lines, since each is 100 minus its partner and both would be plain gray; the report's tables carry them
style = {
	'open': ('var(--ink)', True, 2),#open devices: gray, dashed
	'v6': ('var(--orange)', False, 2),#v6 addresses: orange, solid. Also closed devices with v6 addresses, since closed is the solid side
	'open_v6': ('var(--orange)', True, 2),#open devices with v6 addresses: orange, dashed
}

def legend_item(label, key):#a legend entry drawn in a quantity's style
	stroke, dash, weight = style[key]
	return (stroke, label, dash, weight)

def check_number(v, name):#a value drawn as a coordinate or a size must be a real number; anything else is a bug upstream
	if not isinstance(v, (int, float)) or v != v: raise ValueError(f'{name} is not a number: {v!r}')

def fmt(v):#a coordinate for the file, rounded so the SVG stays readable and small
	return f'{v:.1f}'.rstrip('0').rstrip('.')

def svg_open(name, height, title, description):#the root element, with the figure's color roles declared as CSS variables on it: the site's values when the SVG is inline on the docs site, the light fallbacks otherwise. The title's id is made from the file name, since nine figures share one page and an id must be unique there for aria-labelledby to find the right title
	roles = f"--ink: var(--vp-c-text-1, #3d3d3d); --ink-2: var(--vp-c-text-2, #676767); --grid: var(--vp-c-divider, #e2e2e2); --soft: var(--vp-c-gray-1, #dddddd); --surface: var(--vp-c-bg, #ffffff); --orange: var(--vp-c-brand-1, {orange}); font-family: {font};"
	id = 'title-' + name.removesuffix('.svg')
	return [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" role="img" aria-labelledby="{id}" style="{roles}">', f'<title id="{id}">{title}</title>', f'<desc>{description}</desc>']

def text(x, y, s, size=size_text, anchor='start', fill='var(--ink)', extra='', bold=False):#one run of text in a text token color, never a series color, unless a caller sets a fill for text inside a colored cell; bold for a title or heading
	weight = ' font-weight="700"' if bold else ''
	return f'<text x="{fmt(x)}" y="{fmt(y)}" font-size="{size}" text-anchor="{anchor}" fill="{fill}"{weight} {extra}>{s}</text>'

def wrap(s, size, max_width, bold=False):#one long string broken into lines no wider than max_width, by the per-character estimate for its weight. SVG text has no wrapping of its own, and a footnote or a subtitle has to fit the figure whatever it says
	per = (per_char_bold if bold else per_char)*size
	lines, line = [], ''
	for word in s.split(' '):
		trial = word if not line else line + ' ' + word
		if line and len(trial)*per > max_width: lines.append(line); line = word
		else: line = trial
	if line: lines.append(line)
	return lines

def paragraph(x, y, lines, size=size_text, fill='var(--ink-2)', anchor='start', bold=False):#wrapped lines as one text element, each line a tspan on its own baseline, the first at y
	spans = ''.join(f'<tspan x="{fmt(x)}" y="{fmt(y + i*line_height)}">{line}</tspan>' for i, line in enumerate(lines))
	weight = ' font-weight="700"' if bold else ''
	return f'<text font-size="{size}" text-anchor="{anchor}" fill="{fill}"{weight}>{spans}</text>'

def footer(plot_bottom, foot):#where a footnote's first baseline goes under a plot's x labels, and the figure height that leaves room for all its lines
	return plot_bottom + 44, plot_bottom + 34 + line_height*len(foot)

def rect(x, y, w, h, fill, extra=''):
	return f'<rect x="{fmt(x)}" y="{fmt(y)}" width="{fmt(max(w, 0))}" height="{fmt(max(h, 0))}" fill="{fill}" {extra}/>'

def hairline(x1, y1, x2, y2, stroke='var(--grid)'):#gridlines and axes: one solid hairline, one step off the surface
	return f'<line x1="{fmt(x1)}" y1="{fmt(y1)}" x2="{fmt(x2)}" y2="{fmt(y2)}" stroke="{stroke}" stroke-width="1"/>'

def series_path(points, stroke, dash=False, weight=2):#a data line with round joins and caps, in the stroke, dash, and weight of the style table
	d = 'M ' + ' L '.join(f'{fmt(x)} {fmt(y)}' for x, y in points)
	return f'<path d="{d}" fill="none" stroke="{stroke}" stroke-width="{weight}" stroke-linejoin="round" stroke-linecap="round"{dashes(dash)}/>'

def dashes(dash):#the attribute that makes a line dashed, or nothing
	return ' stroke-dasharray="7 5"' if dash else ''

def hover_points(points, labels):#an invisible, generously sized circle on every point carrying a title, so hovering a line reads the month and value in the browser's own tooltip
	return ''.join(f'<circle cx="{fmt(x)}" cy="{fmt(y)}" r="7" fill="transparent"><title>{label}</title></circle>' for (x, y), label in zip(points, labels))

def legend(items, x, y):#swatch and label pairs: a short 2px line in the series color, then the label in secondary ink. Items flow left to right and wrap to a second row when the next one would run past the figure's right edge, so a long label is never cut off
	parts = []
	start = x
	for stroke, label, *style in items:#a third element, true, draws the swatch dashed; a fourth sets its weight
		needed = 24 + per_char*size_text*len(label)#the item's width at the label size: the swatch, then the label by the regular estimate
		if x > start and x + needed > width - 16: x, y = start, y + 20
		parts.append(f'<line x1="{fmt(x)}" y1="{fmt(y - 4)}" x2="{fmt(x + 18)}" y2="{fmt(y - 4)}" stroke="{stroke}" stroke-width="{style[1] if len(style) > 1 else 2}" stroke-linecap="round"{dashes(style and style[0])}/>')
		parts.append(text(x + 24, y, label, size_text, fill='var(--ink-2)'))
		x += needed + 20
	return ''.join(parts)

def end_margin(labels):#room at a line chart's right for the longest of its end labels, at the text size, with 8 before and 16 after
	return 8 + per_char*size_text*max(len(label) for label in labels) + 16

def spread(pairs, gap=15):#end labels that would overprint, nudged apart: sorted by y, each at least gap below the one above
	out = []
	for key, y in sorted(pairs, key=lambda pair: pair[1]):
		if out and y < out[-1][1] + gap: y = out[-1][1] + gap
		out.append((key, y))
	return out

def month_index(months):#a scale from month strings to x positions: the plot's left edge is the first month, the right edge the last
	return {m: i for i, m in enumerate(months)}

class Plot:#the geometry of one line chart's plot area, so series, axes, and gridlines all agree on where a value lands
	def __init__(self, left, top, right, bottom, x_count, y_low, y_high):
		self.left, self.top, self.right, self.bottom = left, top, right, bottom
		self.x_count, self.y_low, self.y_high = x_count, y_low, y_high
	def x(self, i): return self.left + (self.right - self.left)*i/max(self.x_count - 1, 1)
	def y(self, v): return self.bottom - (self.bottom - self.top)*(v - self.y_low)/(self.y_high - self.y_low)

def axes(plot, y_ticks, x_ticks, y_suffix='%'):#the recessive frame: horizontal hairlines at the y ticks with their values in secondary ink at the left, and x labels in secondary ink below the baseline
	parts = []
	for v in y_ticks:
		parts.append(hairline(plot.left, plot.y(v), plot.right, plot.y(v)))
		parts.append(text(plot.left - 8, plot.y(v) + 4, f'{v:g}{y_suffix}', size_text, 'end', fill='var(--ink-2)', extra='font-variant-numeric="tabular-nums"'))
	for i, label in x_ticks:
		parts.append(hairline(plot.x(i), plot.bottom, plot.x(i), plot.bottom + 4))
		parts.append(text(plot.x(i), plot.bottom + 20, label, size_text, 'middle', fill='var(--ink-2)'))
	return ''.join(parts)

def year_ticks(months):#an x tick at every January, labeled with the year, plus the first month if it isn't one
	ticks = [(i, m[:4]) for i, m in enumerate(months) if m.endswith('-01')]
	if months and not months[0].endswith('-01'): ticks.insert(0, (0, months[0][:4]))
	return ticks

def write(name, lines):#one figure to disk, as lines joined without any blank one
	(folder/name).write_text('\n'.join(line for line in lines if line.strip()) + '\n')
	print(f'wrote charts/{name}')

# The mosaic plot: one month of requests, in the report's sense of the word, the ones people made, as one square. Two columns, open and closed devices, as wide as each kind's share of those requests; within each, v6 addresses on top and v4 below, the cut at that kind's v6 share. Area is requests, so the whole square is the month's requests. Automated requests are left out of the figure on purpose: they are a third of all traffic and say nothing about people, and the table under the figure still carries them. The cells follow the style every figure uses: orange is v6 and gray is v4, and the open column is drawn in stripes on the page background while the closed column is solid, so the cell the report is about is the one of orange stripes. Each cell is labeled with its percent of requests to the nearest whole percent, what it is, and its share of its column; the hover text keeps one decimal
def month_name(m):#2026-09 as September 2026, for a tooltip that reads as speech
	return ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'][int(m[5:]) - 1] + ' ' + m[:4]

def draw_mosaic(c):
	m = c['month']
	label = {'open': 'Open devices, the owner holds root', 'closed': 'Closed devices, the vendor holds root'}
	widths = {'open': c['open_of_person'], 'closed': c['closed_of_person']}
	rates = {'open': c['open_ipv6'], 'closed': c['closed_ipv6']}
	shares = {f'{name}_{family}': widths[name]*(rates[name] if family == 'ipv6' else 100 - rates[name])/100 for name in widths for family in ['ipv4', 'ipv6']}#each cell as a percent of requests
	who = {'open': 'a Windows, macOS, or Linux computer whose owner holds root', 'closed': 'a phone, tablet, Chromebook, or TV where the vendor holds root'}
	left, top, gap = 16, 116, 2
	side = width - 2*left#the plot is a square, as wide as the figure allows
	right, bottom = left + side, top + side
	foot = wrap(f'Orange is v6 addresses and gray is v4. Plain cells are closed devices and striped cells are open devices. Each cell is a percent of the requests people made that month. Automated requests, {c["automated"]:.0f}% of all traffic, are set aside. Source: Cloudflare Radar.', size_text, side)
	height = bottom + 27 + line_height*len(foot)
	lines = svg_open(f'mosaic-{m}.svg', height, f'Web requests in {m}, by kind of device and kind of address', f'A square mosaic of the requests Cloudflare saw in {m}, automated requests set aside, each cell labeled as a percent of those requests. Two columns, on an open device {widths["open"]:.1f}% and on a closed device {widths["closed"]:.1f}%; within each, requests from a v6 address on top, {rates["open"]:.1f}% and {rates["closed"]:.1f}% of the column, and from a v4 address below. The v6 cells are orange and the v4 cells gray; the closed column is plain and the open column is striped, so the cell for an open device and a v6 address is striped orange. The two forces act on that cell from two sides: the closed column squeezes it from the right, and the rising v6 share makes it taller, so its area grows only if it gets taller faster than it gets narrower.')
	lines.append('<defs><pattern id="stripes-v6" patternUnits="userSpaceOnUse" width="11" height="11" patternTransform="rotate(45)"><line x1="0" y1="0" x2="0" y2="11" stroke="var(--orange)" stroke-width="3"/></pattern><pattern id="stripes-v4" patternUnits="userSpaceOnUse" width="11" height="11" patternTransform="rotate(45)"><line x1="0" y1="0" x2="0" y2="11" stroke="var(--soft)" stroke-width="3"/></pattern></defs>')#even diagonal stripes on the page's own background, orange for v6 and gray for v4, thinner than the page between them. The open column's cells are drawn this way, the mosaic's version of the dashed line; the closed column's cells are solid fills in the same two colors
	lines.append(text(left, 24, f'Web requests, {month_name(m)}', size_text, bold=True))
	lines.append(paragraph(left, 44, wrap('Of these requests: how many were made on a device the owner controls, and how many came from a v6 address', size_text, side), size_text))
	x = left
	usable = side - gap
	for name in ['open', 'closed']:
		w = usable*widths[name]/100
		check_number(w, name)
		cut = top + (bottom - top)*rates[name]/100#the y of the line between v6 above and v4 below. v6 is on top so that as its share rises the orange cell grows upward, the way good news moves on a chart, while the closed column squeezes it from the right
		center = x + w/2
		lines.append(text(center, top - 26, label[name], size_text, 'middle', bold=True))
		lines.append(text(center, top - 8, f'{widths[name]:.0f}% of requests', size_text, 'middle', fill='var(--ink-2)'))
		for family, cell_top, cell_height in [('ipv6', top, cut - top - gap/2), ('ipv4', cut + gap/2, bottom - cut - gap/2)]:
			cell, share = shares[name + '_' + family], rates[name] if family == 'ipv6' else 100 - rates[name]
			pretty = 'v6' if family == 'ipv6' else 'v4'
			matters = name == 'open' and family == 'ipv6'#the one cell the report is about: orange for the v6 address, lines for the open device, both at once
			striped = name == 'open'#the open column is striped on the page background, the closed column solid
			if striped: fill = 'url(#stripes-v6)' if family == 'ipv6' else 'url(#stripes-v4)'#orange is v6 addresses and gray is v4, in both columns
			else: fill = 'var(--orange)' if family == 'ipv6' else 'var(--soft)'
			lines.append(rect(x, cell_top, w, cell_height, fill))
			halo = 'paint-order="stroke" stroke="var(--surface)" stroke-width="10" stroke-linejoin="round"' if striped else ''#text on a striped cell floats on a halo of the page's background, so the stripes stop around the numbers: a stroke in the background color painted under the letter fill, 5 units out from every glyph edge
			ink, ink_2 = ('#ffffff', '#ffffff') if family == 'ipv6' and not striped else ('var(--ink)', 'var(--ink-2)')#white text on solid orange, the way the site's brand button is set; dark text on the stripes
			middle = cell_top + cell_height/2
			what = f'{"open" if name == "open" else "closed"} device with {pretty} address'#what the cell is, out of the whole mosaic
			if cell_height > 90:#room for the percent, what the cell is, and its share of the column
				lines.append(text(center, middle - 10, f'{cell:.0f}%', size_number, 'middle', ink, halo))
				lines.append(text(center, middle + 14, what, size_text, 'middle', ink, halo))
				lines.append(text(center, middle + 34, f'{share:.0f}% of {name} devices', size_text, 'middle', fill=ink_2, extra=halo))
			elif cell_height > 30:#one line only
				lines.append(text(center, middle + 5, f'{what}, {cell:.0f}%', size_text, 'middle', ink, halo))
			story = f'In {month_name(m)}, {cell:.1f}% of requests were made on {"an open" if name == "open" else "a closed"} device, {who[name]}, and came from a {pretty} address. That is {share:.1f}% of the requests on {name} devices.'
			if matters: story += ' This is the one cell the report is about: a person, an open device, and an address others can reach.'
			if name == 'open' and family == 'ipv4': story += ' These devices could run a peer, but on this connection they had no routable address to be reached at.'
			lines.append(f'<rect x="{fmt(x)}" y="{fmt(cell_top)}" width="{fmt(w)}" height="{fmt(cell_height)}" fill="transparent"><title>{story}</title></rect>')
		x += w + gap
	lines.append(paragraph(left, bottom + 24 + size_text, foot))
	lines.append('</svg>')
	write(f'mosaic-{m}.svg', lines)

# The cross-tabulation's two margins the report follows and its one cell, month by month, every line a count out of 100 of the month's requests: from v6 addresses, orange; on open devices, dashed; and both at once, dashed orange, running lowest. The complements, v4 addresses and closed devices, are each 100 minus the line drawn and are left to the table under the figure, where they would otherwise be two plain gray lines on top of each other. One denominator for the whole chart
def draw_forces(months, cloudflare):
	name = {'ipv4': 'v4 addresses', 'ipv6': 'v6 addresses', 'open': 'open devices', 'closed': 'closed devices', 'both': 'open devices with v6 addresses'}
	order = [('open', *style['open']), ('ipv6', *style['v6']), ('both', *style['open_v6'])]
	plot = Plot(52, 70, width - end_margin(name[key] for key, *rest in order), 330, len(months), 0, 100)#the right margin fits the longest end label
	foot = wrap('Every line is a count out of 100 requests people made that month; automated requests are set aside. Source: Cloudflare Radar, monthly.', size_text, width - plot.left - 16)
	foot_y, height = footer(plot.bottom, foot)
	lines = svg_open('forces-monthly.svg', height, 'Of every 100 web requests each month: how many from v6 addresses, on open devices, and both', 'Three lines by month, each a count out of 100 requests: from v6 addresses, in orange; on open devices, dashed; and both at once, dashed orange. The complements, v4 addresses and closed devices, are each 100 minus the line drawn. Source: Cloudflare Radar.')
	lines.append(text(plot.left - 36, 22, 'Of every 100 web requests, each month', size_text, bold=True))
	lines.append(paragraph(plot.left - 36, 44, wrap('Orange is v6 addresses. Dashed is open devices. Dashed orange is both at once: open devices with v6 addresses.', size_text, width - 32), size_text))
	lines.append(axes(plot, [0, 25, 50, 75, 100], year_ticks(months)))
	value = {
		'ipv4': lambda c: 100 - c['person_ipv6'],
		'ipv6': lambda c: c['person_ipv6'],
		'open': lambda c: c['open_of_person'],
		'closed': lambda c: c['closed_of_person'],
		'both': lambda c: c['open_of_person']*c['open_ipv6']/100,
	}
	stories = {
		'ipv4': lambda m, v: f'In {month_name(m)}, {v:.1f} of every 100 web requests came from a v4 address.',
		'ipv6': lambda m, v: f'In {month_name(m)}, {v:.1f} of every 100 web requests came from a v6 address, a globally routable address with no translation in the way.',
		'open': lambda m, v: f'In {month_name(m)}, {v:.1f} of every 100 web requests were made on an open device, a Windows, macOS, or Linux computer whose owner holds root.',
		'closed': lambda m, v: f'In {month_name(m)}, {v:.1f} of every 100 web requests were made on a closed device, a phone, tablet, Chromebook, or TV where the vendor holds root.',
		'both': lambda m, v: f'In {month_name(m)}, {v:.1f} of every 100 web requests were made on an open device and came from a v6 address: a person, a device that runs what its owner chooses, and an address others can reach. This is the one cell of the table the report is about.',
	}
	for key, stroke, dash, weight in order:
		points = [(plot.x(i), plot.y(value[key](cloudflare[m]))) for i, m in enumerate(months)]
		lines.append(series_path(points, stroke, dash, weight))
		lines.append(hover_points(points, [stories[key](m, value[key](cloudflare[m])) for m in months]))
	for key, y in spread([(key, plot.y(value[key](cloudflare[months[-1]]))) for key, stroke, dash, weight in order]):
		lines.append(text(plot.right + 8, y + 4, name[key], size_text))
	lines.append(paragraph(plot.left, foot_y, foot))
	lines.append('</svg>')
	write('forces-monthly.svg', lines)

# The share with v6 addresses, month by month, within two groups of requests: on open devices, dashed, and on closed devices, solid, both orange because both count v6 addresses. Each line is a percent of its own group, which is the conventional way to compare groups, and the legend says so on both lines
def draw_rates(months, cloudflare):
	plot = Plot(52, 88, width - 64, 272, len(months), 0, 60)
	foot = wrap('Each line is a percent of its own group of requests that month. Source: Cloudflare Radar, monthly.', size_text, width - plot.left - 16)
	foot_y, height = footer(plot.bottom, foot)
	lines = svg_open('rates-monthly.svg', height, 'Share of requests with v6 addresses, each month', 'Two lines by month, each the share with v6 addresses within one group of requests: on open devices, dashed, and on closed devices, solid. Source: Cloudflare Radar.')
	lines.append(text(plot.left - 36, 22, 'Share of requests with v6 addresses, each month', size_text, bold=True))
	lines.append(legend([legend_item('open devices with v6 addresses', 'open_v6'), legend_item('closed devices with v6 addresses', 'v6')], plot.left, 44))
	lines.append(axes(plot, [0, 20, 40, 60], year_ticks(months)))
	stories = {
		'open_ipv6': lambda m, v: f'In {month_name(m)}, of requests on open devices, {v:.1f}% came from a v6 address and {100 - v:.1f}% from a v4 address. This is the figure a peer-to-peer operator cares about: of the devices that could run a peer, how many held a routable address.',
		'closed_ipv6': lambda m, v: f'In {month_name(m)}, of requests on closed devices, phones, tablets, Chromebooks, and TVs, {v:.1f}% came from a v6 address. Phones ride cellular carriers, who moved to v6 early, which is why this line runs above the open-device one.',
	}
	for key, stroke, dash, weight in [('closed_ipv6', *style['v6']), ('open_ipv6', *style['open_v6'])]:
		points = [(plot.x(i), plot.y(cloudflare[m][key])) for i, m in enumerate(months)]
		lines.append(series_path(points, stroke, dash, weight))
		lines.append(hover_points(points, [stories[key](m, cloudflare[m][key]) for m in months]))
		lines.append(text(plot.right + 8, points[-1][1] + 4, f'{cloudflare[months[-1]][key]:.0f}%', size_text))
	lines.append(paragraph(plot.left, foot_y, foot))
	lines.append('</svg>')
	write('rates-monthly.svg', lines)

# The squares: the chance that two requests drawn at random both have a property, which is that property's share squared. Two figures on one axis, because they draw from different pools. The first draws from every request: both on open devices, both from v6 addresses, and both open and v6, each in the style of the share it squares. The second draws from open devices only, the swarm's pool, where the one question left is the address: both v6, which is the open-device v6 share squared
def draw_pairs(filename, title, items, months, cloudflare):
	plot = Plot(52, 52, width - end_margin(f'00.0%  {label}' for key, rate_key, label, look, story in items), 282, len(months), 0, 25)#the right margin fits the longest end label, value and name
	foot = wrap('Each line is a share of requests squared: the chance that two requests drawn at random from the month both have it. Source: Cloudflare Radar, monthly.', size_text, width - plot.left - 16)
	foot_y, height = footer(plot.bottom, foot)
	lines = svg_open(filename, height, title, 'Lines by month, each a share squared: ' + ', '.join(label for key, rate_key, label, look, story in items) + '. Source: Cloudflare Radar.')
	lines.append(text(plot.left - 36, 22, title, size_text, bold=True))
	lines.append(axes(plot, [0, 5, 10, 15, 20, 25], year_ticks(months)))
	for key, rate_key, label, look, story in items:
		points = [(plot.x(i), plot.y(cloudflare[m][key])) for i, m in enumerate(months)]
		lines.append(series_path(points, *look))
		lines.append(hover_points(points, [story(m, cloudflare[m][key], cloudflare[m][rate_key]) for m in months]))
	for key, y in spread([(key, plot.y(cloudflare[months[-1]][key])) for key, rate_key, label, look, story in items]):#each line named at its right end, with its last value to one decimal
		label = next(label for k, rate_key, label, look, story in items if k == key)
		lines.append(text(plot.right + 8, y + 4, f'{cloudflare[months[-1]][key]:.1f}%  {label}', size_text))
	lines.append(paragraph(plot.left, foot_y, foot))
	lines.append('</svg>')
	write(filename, lines)

# The long view: v6, of Google's users, since 2008, beside desktop, of StatCounter's page views, since 2009. The two forces over seventeen years, each from the source that has watched it longest, with the span the Cloudflare detail covers marked
def draw_history(google, statcounter, cloudflare_months):
	months = sorted(set(google) | set(statcounter))
	months = [m for m in months if m >= '2008-09' and m <= cloudflare_months[-1]]
	index = month_index(months)
	plot = Plot(52, 70, width - 64, 254, len(months), 0, 100)
	foot = wrap('Monthly means of daily values (Google) and monthly figures (StatCounter). The shaded span is where the Cloudflare detail begins.', size_text, width - plot.left - 16)
	foot_y, height = footer(plot.bottom, foot)
	lines = svg_open('history.svg', height, 'The two forces since 2008', 'Two lines by month since 2008: the share of Google users on v6, and the share of StatCounter page views from a desktop.')
	lines.append(text(plot.left - 36, 22, 'Since 2008: v6 addresses at Google, desktops at StatCounter', size_text, bold=True))
	lines.append(legend([legend_item('desktops, StatCounter page views', 'open'), legend_item('v6 addresses, Google users', 'v6')], plot.left, 44))
	first, last = index[cloudflare_months[0]], index[cloudflare_months[-1]]
	lines.append(rect(plot.x(first), plot.top, plot.x(last) - plot.x(first), plot.bottom - plot.top, 'var(--soft)', 'opacity="0.35"'))
	lines.append(text(plot.x(first) - 6, plot.top + 12, 'Cloudflare detail', size_text, 'end', fill='var(--ink-2)'))
	lines.append(axes(plot, [0, 25, 50, 75, 100], [(index[m], m[:4]) for m in months if m.endswith('-01') and int(m[:4]) % 2 == 0]))
	stories = {
		'desktop': lambda m, v: f'In {month_name(m)}, {v:.1f}% of the page views StatCounter saw across its sites came from desktop and laptop computers, and {100 - v:.1f}% from phones and tablets. This is the closest long record of the share of a person\u2019s activity on open devices.',
		'total': lambda m, v: f'In {month_name(m)}, {v:.1f}% of the people using Google reached it over v6, as a monthly mean of the daily figure Google publishes. In 2008 this was a fraction of a percent.',
	}
	for source, key, look in [(statcounter, 'desktop', style['open']), (google, 'total', style['v6'])]:
		series = [(index[m], source[m][key]) for m in months if m in source]
		points = [(plot.x(i), plot.y(v)) for i, v in series]
		lines.append(series_path(points, *look))
		lines.append(hover_points(points, [stories[key](months[i], v) for i, v in series]))
		lines.append(text(plot.right + 8, points[-1][1] + 4, f'{series[-1][1]:.1f}%', size_text))
	lines.append(paragraph(plot.left, foot_y, foot))
	lines.append('</svg>')
	write('history.svg', lines)

# Small multiples for a cross-check: one panel per source, the same months and the same axis in every panel, one series each, so the eye compares levels and shapes without a color per source. Each panel carries its own tooltip wording, since each source counts something different
def draw_multiples(name, title, description, panels, months, y_low, y_high, y_ticks, looks, footnote, decimals=1):#looks is one style tuple per panel
	columns = 2
	rows = (len(panels) + columns - 1)//columns
	panel_w, panel_h, top_pad = (width - 32)/columns, 170, 40
	foot = wrap(footnote, size_text, width - 32)
	foot_y = top_pad + rows*panel_h + 10
	height = foot_y + line_height*(len(foot) - 1) + 8
	lines = svg_open(name, height, title, description)
	lines.append(text(16, 22, title, size_text, bold=True))
	for n, (label, series, story) in enumerate(panels):
		px, py = 16 + (n % columns)*panel_w, top_pad + (n//columns)*panel_h
		label_lines = wrap(label, size_text, panel_w - 48, bold=True)#a panel label can be long, so it wraps, at the bold estimate since it is set bold, and the plot below it starts lower
		plot = Plot(px + 40, py + 14 + line_height*len(label_lines), px + panel_w - 56, py + panel_h - 36, len(months), y_low, y_high)
		lines.append(paragraph(plot.left, py + 12, label_lines, size_text, fill='var(--ink)', bold=True))
		lines.append(axes(plot, y_ticks, year_ticks(months)))
		present = [m for m in months if m in series and series[m] != '']
		points = [(plot.x(months.index(m)), plot.y(series[m])) for m in present]
		if not points: continue
		lines.append(series_path(points, *looks[n]))
		lines.append(hover_points(points, [story(m, series[m]) for m in present]))
		lines.append(text(plot.right + 6, points[-1][1] + 4, f'{series[present[-1]]:.{decimals}f}%', size_text))
	lines.append(paragraph(16, foot_y, foot))
	lines.append('</svg>')
	write(name, lines)

def main():
	global table
	path = Path(__file__).resolve().parent/'data'/'table.json'
	if not path.exists(): raise FileNotFoundError('no data/table.json; run fetch.py then table.py first')
	table = json.loads(path.read_text())
	folder.mkdir(exist_ok=True)
	months, cloudflare, latest = table['months'], table['cloudflare'], table['latest']
	google, apnic, statcounter, wikimedia = table['google'], table['apnic'], table['statcounter'], table['wikimedia']

	draw_mosaic(cloudflare[latest])
	draw_forces(months, cloudflare)
	draw_rates(months, cloudflare)
	draw_pairs('square-monthly.svg', 'Picking two requests at random, the chance the pair are:', [
		('open_squared', 'open_of_person', 'both open', style['open'], lambda m, v, r: f'In {month_name(m)}, pick two requests at random: there was a {v:.1f}% chance both were made on open devices, so that both people could choose their own software, because {r:.1f}% of requests were on open devices that month and {r:.1f}% squared is {v:.1f}%.'),
		('person_ipv6_squared', 'person_ipv6', 'both v6', style['v6'], lambda m, v, r: f'In {month_name(m)}, pick two requests at random: there was a {v:.1f}% chance both came from v6 addresses, addresses built to be reached, because {r:.1f}% of requests came from a v6 address that month and {r:.1f}% squared is {v:.1f}%.'),
		('open_ipv6_of_person_squared', 'open_ipv6_of_person', 'both open and v6', style['open_v6'], lambda m, v, r: f'In {month_name(m)}, pick two requests at random: there was a {v:.1f}% chance both were on an open device and from a v6 address, because {r:.1f}% of requests were both that month and {r:.1f}% squared is {v:.1f}%. This is the chance that two people taken at random from the web could each run a peer and reach the other over v6.'),
	], months, cloudflare)
	draw_pairs('square-open.svg', 'Picking two open device requests at random, the chance the pair are:', [
		('open_ipv6_squared', 'open_ipv6', 'both v6', style['open_v6'], lambda m, v, r: f'In {month_name(m)}, pick two requests on open devices at random: there was a {v:.1f}% chance both came from v6 addresses, because {r:.1f}% of requests on open devices came from a v6 address that month and {r:.1f}% squared is {v:.1f}%. Two peers need both ends on v6 to connect that way, so this is the chance a random pair of peers in a swarm could.'),
	], months, cloudflare)
	draw_history(google, statcounter, months)
	systems = [('WINDOWS', 'Windows'), ('IOS', 'iOS'), ('MACOSX', 'macOS'), ('ANDROID', 'Android'), ('LINUX', 'Linux'), ('CHROMEOS', 'ChromeOS')]#interleaved so the open systems fill the left column of the grid and the closed ones the right
	draw_multiples('ipv6-by-system.svg', 'Share with v6 addresses, by operating system', 'Six panels, one per operating system, each the share of requests on that system that came from a v6 address, by month: Windows, macOS, Linux, iOS, Android, ChromeOS.', [
		(pretty, {m: cloudflare[m]['system_ipv6'][key] for m in months}, (lambda pretty: lambda m, v: f'In {month_name(m)}, of requests on {pretty}, {v:.1f}% came from a v6 address and {100 - v:.1f}% from a v4 address.')(pretty))
		for key, pretty in systems
	], months, 10, 60, [10, 20, 30, 40, 50, 60], [style['open_v6'], style['v6']]*3, 'Each panel is one operating system on the same axis. Open systems on the left, dashed; closed on the right, solid. Source: Cloudflare Radar, monthly.', decimals=0)
	draw_multiples('check-ipv6.svg', 'Share with v6 addresses, by four sources', 'Four panels, one per source, each the share on v6 by month: all traffic at Cloudflare with automated requests included, requests at Cloudflare, Google users, APNIC users able to use v6.', [
		('All traffic, automated included, Cloudflare', {m: cloudflare[m]['all_ipv6'] for m in months}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of all traffic Cloudflare saw, automated requests included, came from a v6 address. Bots run mostly on v4 from data centers, which is why this panel sits below the one for requests alone.'),
		('Requests, Cloudflare', {m: cloudflare[m]['person_ipv6'] for m in months}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of requests at Cloudflare came from a v6 address. This is a share of requests: a device that makes many requests counts many times.'),
		('Users, Google', {m: google[m]['total'] for m in months if m in google}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of the people using Google reached it over v6, as a monthly mean of Google\u2019s daily figure. This counts users, not requests, and anyone who reached Google over v6 at all.'),
		('Users able to use v6, APNIC', {m: apnic[m]['capable'] for m in months if m in apnic}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of the users APNIC sampled through its advertisement-delivered test could fetch over v6, a sample-weighted mean over the month. Slightly fewer, about two points, preferred v6 when offered both.'),
	], months, 20, 60, [20, 40, 60], [style['v6']]*4, 'Shares of requests above, shares of users below. Each panel is one source on the same axis.')
	draw_multiples('check-open.svg', 'Share on open devices or desktops, by four measures', 'Four panels, one per measure, each the share of activity on open devices or desktops by month: Cloudflare by operating system, Cloudflare by device type, StatCounter page views from a desktop, Wikimedia page views through the desktop site.', [
		('Open devices, Cloudflare requests, by operating system', {m: cloudflare[m]['open_of_person'] for m in months}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of requests at Cloudflare were made on an open device, Windows, macOS, or Linux, counting only requests whose operating system Cloudflare recognized. This is the report\u2019s own definition.'),
		('Desktops, Cloudflare requests, by device type', {m: cloudflare[m]['desktop_of_person'] for m in months}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of requests at Cloudflare were made on a device it classes as desktop. This classifier covers nearly every request, so where this panel and the one to its left differ, the difference is requests with no recognized operating system.'),
		('Desktops, StatCounter page views', {m: statcounter[m]['desktop'] for m in months if m in statcounter}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of the page views StatCounter saw across more than a million sites came from desktop and laptop computers. A noisy series month to month, and the longest one of these four.'),
		('Desktop site, Wikimedia page views', {m: wikimedia[m]['desktop_of_user'] for m in months if m in wikimedia}, lambda m, v: f'In {month_name(m)}, {v:.1f}% of page views at Wikimedia, across all its projects and counting only those its agent classifier marks as a person\u2019s, came through the desktop site rather than the mobile site or the apps.'),
	], months, 20, 60, [20, 40, 60], [style['open']]*4, 'Shares of requests at Cloudflare, of page views at StatCounter and Wikimedia. Each panel is one measure on the same axis.')

if __name__ == '__main__':
	try: main()
	except Exception as e:
		print('chart.py stopped: ' + repr(e), file=sys.stderr)
		sys.exit(1)
