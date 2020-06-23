#include "LCDWIKI_font.c"
#include "LCDWIKI_GUI.h"


// Miscellaneous

void LCDWIKI_GUI::drawBorder(int x, int y, int w, int h, int b, int color)
	// draw a frigin border
{
	Fill_Rect(x,		y,		b,		h,	color);
	Fill_Rect(x+w-b,	y,		b,		h,	color);
	Fill_Rect(x,		y,		w,		b,	color);
	Fill_Rect(x,		y+h-b,	w,		b,	color);
}


//------------------------------------------------------
// ILI9431 FONT STUFF
//------------------------------------------------------

#ifndef WITH_ILI9431_FONTS
	// prh - already defined in Paul's stuff
	#define swap(a, b) { int16_t t = a; a = b; b = t; }
#endif


#if WITH_ILI9431_FONTS
	#include <myDebug.h>

	static uint32_t fetchbit(const uint8_t *p, uint32_t index)
	{
		if (p[index >> 3] & (1 << (7 - (index & 7)))) return 1;
		return 0;
	}
	
	static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required)
	{
		uint32_t val = 0;
		do {
			uint8_t b = p[index >> 3];
			uint32_t avail = 8 - (index & 7);
			if (avail <= required) {
				val <<= avail;
				val |= b & ((1 << avail) - 1);
				index += avail;
				required -= avail;
			} else {
				b >>= avail - required;
				val <<= required;
				val |= b & ((1 << required) - 1);
				break;
			}
		} while (required);
		return val;
	}
	
	static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required)
	{
		uint32_t val = fetchbits_unsigned(p, index, required);
		if (val & (1 << (required - 1))) {
			return (int32_t)val - (1 << required);
		}
		return (int32_t)val;
	}


	void LCDWIKI_GUI::drawFontChar(unsigned int c)
	{
		uint32_t bitoffset;
		const uint8_t *data;
	
		//Serial.printf("drawFontChar %d\n", c);
	
		if (c >= font->index1_first && c <= font->index1_last)
		{
			bitoffset = c - font->index1_first;
			bitoffset *= font->bits_index;
		}
		else if (c >= font->index2_first && c <= font->index2_last)
		{
			bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
			bitoffset *= font->bits_index;
		}
		else if (font->unicode)
		{
			return; // TODO: implement sparse unicode
		}
		else
		{
			return;
		}
		
		//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
		
		data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);
		uint32_t encoding = fetchbits_unsigned(data, 0, 3);
		if (encoding != 0) return;
		
		uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
		bitoffset = font->bits_width + 3;
		uint32_t height = fetchbits_unsigned(data, bitoffset, font->bits_height);
		bitoffset += font->bits_height;
		//Serial.printf("  size =   %d,%d\n", width, height);
	
		int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
		bitoffset += font->bits_xoffset;
		int32_t yoffset = fetchbits_signed(data, bitoffset, font->bits_yoffset);
		bitoffset += font->bits_yoffset;
		//Serial.printf("  offset = %d,%d\n", xoffset, yoffset);
	
		uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
		bitoffset += font->bits_delta;
		//Serial.printf("  delta =  %d\n", delta);
	
		//Serial.printf("  cursor = %d,%d\n", cursor_x, cursor_y);
	
		// horizontally, we draw every pixel, or none at all
		if (text_x < 0) text_x = 0;
		int32_t origin_x = text_x + xoffset;
		if (origin_x < 0)
		{
			text_x -= xoffset;
			origin_x = 0;
		}
		if (origin_x + (int)width > Get_Width())
		{
			// if (!wrap) return;  prh - wrap
			
			origin_x = 0;
			if (xoffset >= 0) {
				text_x = 0;
			} else {
				text_x = -xoffset;
			}
			text_y += font->line_space;
		}
		if (text_y >= Get_Height()) return;
		text_x += delta;
	
		// vertically, the top and/or bottom can be clipped
		int32_t origin_y = text_y + font->cap_height - height - yoffset;
		//Serial.printf("  origin = %d,%d\n", origin_x, origin_y);
	
		// TODO: compute top skip and number of lines
		int32_t linecount = height;
		//uint32_t loopcount = 0;
		uint32_t y = origin_y;
		while (linecount)
		{
			//Serial.printf("    linecount = %d\n", linecount);
			uint32_t b = fetchbit(data, bitoffset++);
			if (b == 0)
			{
				//Serial.println("    single line");
				uint32_t x = 0;
				do
				{
					uint32_t xsize = width - x;
					if (xsize > 32) xsize = 32;
					uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
					drawFontBits(bits, xsize, origin_x + x, y, 1);
					bitoffset += xsize;
					x += xsize;
				} while (x < width);
				y++;
				linecount--;
			}
			else
			{
				uint32_t n = fetchbits_unsigned(data, bitoffset, 3) + 2;
				bitoffset += 3;
				uint32_t x = 0;
				do
				{
					uint32_t xsize = width - x;
					if (xsize > 32) xsize = 32;
					//Serial.printf("    multi line %d\n", n);
					uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
					drawFontBits(bits, xsize, origin_x + x, y, n);
					bitoffset += xsize;
					x += xsize;
				} while (x < width);
				y += n;
				linecount -= n;
			}
			//if (++loopcount > 100) {
				//Serial.println("     abort draw loop");
				//break;
			//}
		}
	}
	
	
	
	void LCDWIKI_GUI::drawFontBits(uint32_t bits, uint32_t numbits, uint32_t x, uint32_t y, uint32_t repeat)
	{
		#if 1			
			// TODO: replace this *slow* code with something fast...
			//Serial.printf("      %d bits at %d,%d: %X\n", numbits, x, y, bits);
			if (bits == 0) return;
			do {
				uint32_t x1 = x;
				uint32_t n = numbits;
				do
				{
					n--;
					if (bits & (1 << n))
					{
						Draw_Pixe(x1, y, text_color);
						//Serial.printf("        pixel at %d,%d\n", x1, y);
					}
					x1++;
				} while (n > 0);
				y++;
				repeat--;
			} while (repeat);
		#endif
		
		
		
		#if 0
			if (bits == 0) return;
			beginSPITransaction();
			int w = 0;
			do {
				uint32_t x1 = x;
				uint32_t n = numbits;		
				
				writecommand_cont(ILI9341_PASET); // Row addr set
				writedata16_cont(y);   // YSTART
				writedata16_cont(y);   // YEND	
				
				do {
					n--;
					if (bits & (1 << n)) {
						w++;
					}
					else if (w > 0) {
						// "drawFastHLine(x1 - w, y, w, textcolor)"
						writecommand_cont(ILI9341_CASET); // Column addr set
						writedata16_cont(x1 - w);   // XSTART
						writedata16_cont(x1);   // XEND					
						writecommand_cont(ILI9341_RAMWR);
						while (w-- > 1) { // draw line
							writedata16_cont(textcolor);
						}
						writedata16_last(textcolor);
					}
								
					x1++;
				} while (n > 0);
		
				if (w > 0) {
						// "drawFastHLine(x1 - w, y, w, textcolor)"
						writecommand_cont(ILI9341_CASET); // Column addr set
						writedata16_cont(x1 - w);   // XSTART
						writedata16_cont(x1);   // XEND
						writecommand_cont(ILI9341_RAMWR);				
						while (w-- > 1) { //draw line
							writedata16_cont(textcolor);
						}
						writedata16_last(textcolor);
				}
				
				y++;
				repeat--;
			} while (repeat);
			endSPITransaction();
		#endif	
	}

#endif	// WITH_ILI9431_FONTS



//------------------------------------------
// text extents 
//-----------------------------------------


int LCDWIKI_GUI::getFontHeight()
{
	#if WITH_ILI9431_FONTS
		if (font)
			return font->line_space;
	#endif
	
	return text_size * 8;
}


int LCDWIKI_GUI::getTextExtent(const char *text)
{
	int len = strlen(text);

	#if WITH_ILI9431_FONTS
		if (font)
		{
			int strlen = 0;
			for (int i=0; i<len; i++)
			{
				unsigned int c = text[i];
				uint32_t bitoffset;
				if (c >= font->index1_first && c <= font->index1_last)
				{
					bitoffset = c - font->index1_first;
					bitoffset *= font->bits_index;
				}
				else if (c >= font->index2_first && c <= font->index2_last)
				{
					bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
					bitoffset *= font->bits_index;
				}
				else
				{
					warning(0,"WARNING: chr(%d)=%c cannot be mapped",c,c);
					continue;
				}

				const uint8_t *data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);
				uint32_t encoding = fetchbits_unsigned(data, 0, 3);
				if (encoding != 0)
				{
					warning(0,"WARNING: chr(%d)=%c bad encoding(%d)",c,c,encoding);
					continue;
				}

				bitoffset = font->bits_width + 3;
				bitoffset += font->bits_height;
				bitoffset += font->bits_xoffset;
				bitoffset += font->bits_yoffset;
				
				uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
				strlen += (int) delta;				
			}
			return strlen;
		}
	#endif
	
	return len * text_size * 6;
}



//-----------------------------------------
// printf_justified()
//-----------------------------------------
// prints multi-line strings justified within the bounding box
// without clipping.  clears the entire bounding box if use_bc.


// #define LCD_JUST_LEFT    0
// #define LCD_JUST_CENTER  1 
// #define LCD_JUST_RIGHT   2



#define MAX_PRINTF_STRING  1024

void LCDWIKI_GUI::printf_justified(
	int x,
	int y,
	int w,
	int h,
	int just,
	uint16_t fc,			// calls Set_Text_colour
	uint16_t bc,			// draws the rectangle for you if font*
	bool use_bc,
	const char *format,
	...)
//
{
	char display_buffer[MAX_PRINTF_STRING+1];
	
	va_list var;
	va_start(var, format);
        
	// allows for 2 times expansion, but no telling - it may just crash!!
	
	if (strlen(format) >= MAX_PRINTF_STRING/2)
	{
		my_error("error - MAX_PRINTF_STRING overflow",0);
		return;
	}
    
	vsprintf(display_buffer,format,var);

	if (use_bc)
		Fill_Rect(x,y-1,w,h+1,bc);
		
	// print it by lines

	Set_Text_colour(fc);
	int yoffset = getFontHeight();
	char *to_print = display_buffer;
	char *end = to_print;
	
	while (*to_print)
	{
		if (!*end || *end == '\n')
		{
			if (*end)
				*end++ = 0;

			// print a line
			// getTextExtent is not really working correctly at this time.
			// Right justify is definitely not working

			int use_x = x;			
			if (just != LCD_JUST_LEFT)
			{
				int width = getTextExtent(to_print);
				int xoffset = (w - width);
				if (xoffset < 0) xoffset = 0;
				if (just == LCD_JUST_CENTER)
					xoffset /= 2;
				use_x += xoffset;
			}
			
			Print_String(to_print,use_x,y);
			
			// next lne
			y += yoffset;
			to_print = end;
		}
		else
			end++;
	}
}




