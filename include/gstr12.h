/* GeoWorks 1.2 GString-Opcodes (0..89). Quelle: FreeGEOS Library/Convert/
 * convertGStringTables.asm, gegen PS.GEO-Sprungtabelle geprueft.
 * Praefix G12_ statt GR_, um Verwechslung mit den 2.0-Werten zu vermeiden. */
#ifndef GSTR12_H
#define GSTR12_H

#define G12_END_STRING                     0  /* 0x00 */
#define G12_COMMENT                        1  /* 0x01 */
#define G12_NULL_OP                        2  /* 0x02 */
#define G12_ESCAPE                         3  /* 0x03 */
#define G12_SAVE_STATE                     4  /* 0x04 */
#define G12_RESTORE_STATE                  5  /* 0x05 */
#define G12_NEW_PAGE                       6  /* 0x06 */
#define G12_APPLY_ROTATION                 7  /* 0x07 */
#define G12_APPLY_SCALE                    8  /* 0x08 */
#define G12_APPLY_TRANSLATION              9  /* 0x09 */
#define G12_SET_TRANSFORM                 10  /* 0x0a */
#define G12_APPLY_TRANSFORM               11  /* 0x0b */
#define G12_SET_NULL_TRANSFORM            12  /* 0x0c */
#define G12_DRAW_LINE                     13  /* 0x0d */
#define G12_DRAW_LINE_TO                  14  /* 0x0e */
#define G12_DRAW_RECT                     15  /* 0x0f */
#define G12_DRAW_RECT_TO                  16  /* 0x10 */
#define G12_DRAW_HLINE                    17  /* 0x11 */
#define G12_DRAW_HLINE_TO                 18  /* 0x12 */
#define G12_DRAW_VLINE                    19  /* 0x13 */
#define G12_DRAW_VLINE_TO                 20  /* 0x14 */
#define G12_DRAW_ROUND_RECT               21  /* 0x15 */
#define G12_DRAW_ROUND_RECT_TO            22  /* 0x16 */
#define G12_DRAW_POINT                    23  /* 0x17 */
#define G12_DRAW_POINT_CP                 24  /* 0x18 */
#define G12_DRAW_BITMAP                   25  /* 0x19 */
#define G12_DRAW_BITMAP_CP                26  /* 0x1a */
#define G12_DRAW_BITMAP_PTR               27  /* 0x1b */
#define G12_DRAW_BITMAP_OPTR              28  /* 0x1c */
#define G12_DRAW_CHAR                     29  /* 0x1d */
#define G12_DRAW_CHAR_CP                  30  /* 0x1e */
#define G12_DRAW_TEXT                     31  /* 0x1f */
#define G12_DRAW_TEXT_CP                  32  /* 0x20 */
#define G12_DRAW_TEXT_FIELD               33  /* 0x21 */
#define G12_DRAW_TEXT_PTR                 34  /* 0x22 */
#define G12_DRAW_POLYLINE                 35  /* 0x23 */
#define G12_DRAW_ELLIPSE                  36  /* 0x24 */
#define G12_DRAW_ARC                      37  /* 0x25 */
#define G12_DRAW_SPLINE                   38  /* 0x26 */
#define G12_DRAW_POLYGON                  39  /* 0x27 */
#define G12_FILL_RECT                     40  /* 0x28 */
#define G12_FILL_RECT_TO                  41  /* 0x29 */
#define G12_FILL_ROUND_RECT               42  /* 0x2a */
#define G12_FILL_ROUND_RECT_TO            43  /* 0x2b */
#define G12_FILL_ARC                      44  /* 0x2c */
#define G12_FILL_POLYGON                  45  /* 0x2d */
#define G12_FILL_ELLIPSE                  46  /* 0x2e */
#define G12_SET_DRAW_MODE                 47  /* 0x2f */
#define G12_REL_MOVE_TO                   48  /* 0x30 */
#define G12_MOVE_TO                       49  /* 0x31 */
#define G12_SET_LINE_COLOR                50  /* 0x32 */
#define G12_SET_LINE_MASK                 51  /* 0x33 */
#define G12_SET_LINE_COLOR_MAP            52  /* 0x34 */
#define G12_SET_LINE_WIDTH                53  /* 0x35 */
#define G12_SET_LINE_JOIN                 54  /* 0x36 */
#define G12_SET_LINE_END                  55  /* 0x37 */
#define G12_SET_LINE_ATTR                 56  /* 0x38 */
#define G12_SET_MITER_LIMIT               57  /* 0x39 */
#define G12_SET_LINE_STYLE                58  /* 0x3a */
#define G12_SET_AREA_COLOR                59  /* 0x3b */
#define G12_SET_AREA_MASK                 60  /* 0x3c */
#define G12_SET_AREA_COLOR_MAP            61  /* 0x3d */
#define G12_SET_AREA_ATTR                 62  /* 0x3e */
#define G12_SET_TEXT_COLOR                63  /* 0x3f */
#define G12_SET_TEXT_MASK                 64  /* 0x40 */
#define G12_SET_TEXT_COLOR_MAP            65  /* 0x41 */
#define G12_SET_TEXT_STYLE                66  /* 0x42 */
#define G12_SET_TEXT_MODE                 67  /* 0x43 */
#define G12_SET_TEXT_SPACE_PAD            68  /* 0x44 */
#define G12_SET_TEXT_ATTR                 69  /* 0x45 */
#define G12_SET_FONT                      70  /* 0x46 */
#define G12_SET_STRING_BOUNDS             71  /* 0x47 */
#define G12_CREATE_PALETTE                72  /* 0x48 */
#define G12_DESTROY_PALETTE               73  /* 0x49 */
#define G12_SET_PALETTE_ENTRY             74  /* 0x4a */
#define G12_SET_PALETTE                   75  /* 0x4b */
#define G12_SET_BORDER_WIDTH              76  /* 0x4c */
#define G12_SET_BORDER_JOIN               77  /* 0x4d */
#define G12_SET_LINE_COLOR_INDEX          78  /* 0x4e */
#define G12_SET_CUSTOM_LINE_MASK          79  /* 0x4f */
#define G12_SET_AREA_COLOR_INDEX          80  /* 0x50 */
#define G12_SET_CUSTOM_AREA_MASK          81  /* 0x51 */
#define G12_SET_TEXT_COLOR_INDEX          82  /* 0x52 */
#define G12_SET_CUSTOM_TEXT_MASK          83  /* 0x53 */
#define G12_SET_CUSTOM_LINE_STYLE         84  /* 0x54 */
#define G12_SET_TRACK_KERN                85  /* 0x55 */
#define G12_INIT_DEFAULT_TRANSFORM        86  /* 0x56 */
#define G12_SET_DEFAULT_TRANSFORM         87  /* 0x57 */
#define G12_SET_CLIP_RECT                 88  /* 0x58 */
#define G12_SET_DOC_CLIP_RECT             89  /* 0x59 */

#define G12_FIRST_OUTPUT                  13
#define G12_LAST_OUTPUT                   46
#define G12_NUM_OPCODES                   90

#endif
