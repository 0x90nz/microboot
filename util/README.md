# bdf2bin

This python script will convert bdf fonts into a format loadable by μboot.

The file format for fonts is fairly simple, with a 4 byte header, followed
immediately by all the bitmaps which compose the font.

Fonts are always monospaced, and thus use a single value for the width and
height of each character.

The header is comprised of 4 bytes. The first byte is the width of the font,
in pixels, the second byte is the height of the font in pixels. The remaining
two bytes are unused, but are reserved - either for new values, or more likely
for a magic value.

The bitmaps start from the ASCII space (' ') character, and go until the end of
the character set. Essentially, the bitmaps are indexed by their ASCII character
codes with an offset of -32.

The bitmaps are comprised of single-bit values, 1 indicates that the foreground
colour should be drawn, and zero indicates that the background colour should be
drawn. The bits of each byte correspond to a pixel, with the MSB being on the
left and going to the LSB on the right. Each line must take at least one byte -
lines are not packed. For example: if a font is 5 pixels wide and 8 pixels high,
each line will take a single byte (with the highest 5 bits being used), and each
character will take 8 bytes (one for each line). If the characters are more than
8 pixels wide, then a line is stored consecutively (again, not packed). For
example, a font 13 pixels by 20 would take two bytes per line, and 40 bytes
total (two bytes by 20 lines).

