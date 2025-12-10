#ifndef __HZ_FONT_H__
#define __HZ_FONT_H__

#include <stdint.h>

/* 三个汉字的 16x16 点阵，每个 32 字节 */
extern const uint8_t hz_1_16x16[32];
extern const uint8_t hz_2_16x16[32];
extern const uint8_t hz_3_16x16[32];

/* 绘制 16x16 汉字 */
void lcd_draw_hz16(int x, int y, const uint8_t glyph[32]);

#endif /* __HZ_FONT_H__ */
