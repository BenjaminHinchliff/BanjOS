#include "keycodes.h"

char scan_code_to_char(enum KeyCode scan_code) {
  switch (scan_code) {
  case KEYCODE_A_PRESSED:
    return 'a';
  case KEYCODE_BACKSLASH_PRESSED:
    return '\\';
  case KEYCODE_B_PRESSED:
    return 'b';
  case KEYCODE_COMMA_PRESSED:
    return ',';
  case KEYCODE_C_PRESSED:
    return 'c';
  case KEYCODE_D_PRESSED:
    return 'd';
  case KEYCODE_EIGHT_PRESSED:
    return '8';
  case KEYCODE_EQUALS_PRESSED:
    return '=';
  case KEYCODE_E_PRESSED:
    return 'e';
  case KEYCODE_FIVE_PRESSED:
    return '5';
  case KEYCODE_FOUR_PRESSED:
    return '4';
  case KEYCODE_F_PRESSED:
    return 'f';
  case KEYCODE_GRAVE_ACCENT_PRESSED:
    return '`';
  case KEYCODE_G_PRESSED:
    return 'g';
  case KEYCODE_H_PRESSED:
    return 'h';
  case KEYCODE_I_PRESSED:
    return 'i';
  case KEYCODE_J_PRESSED:
    return 'j';
  case KEYCODE_KEYPAD_EIGHT_PRESSED:
    return '8';
  case KEYCODE_KEYPAD_FIVE_PRESSED:
    return '5';
  case KEYCODE_KEYPAD_FOUR_PRESSED:
    return '4';
  case KEYCODE_KEYPAD_MINUS_PRESSED:
    return '-';
  case KEYCODE_KEYPAD_NINE_PRESSED:
    return '9';
  case KEYCODE_KEYPAD_ONE_PRESSED:
    return '1';
  case KEYCODE_KEYPAD_PERIOD_PRESSED:
    return '.';
  case KEYCODE_KEYPAD_SEVEN_PRESSED:
    return '7';
  case KEYCODE_KEYPAD_SIX_PRESSED:
    return '6';
  case KEYCODE_KEYPAD_STAR_PRESSED:
    return '*';
  case KEYCODE_KEYPAD_THREE_PRESSED:
    return '3';
  case KEYCODE_KEYPAD_TWO_PRESSED:
    return '2';
  case KEYCODE_KEYPAD_ZERO_PRESSED:
    return '0';
  case KEYCODE_K_PRESSED:
    return 'k';
  case KEYCODE_LEFT_BRACKET_PRESSED:
    return '[';
  case KEYCODE_L_PRESSED:
    return 'l';
  case KEYCODE_MINUS_PRESSED:
    return '-';
  case KEYCODE_M_PRESSED:
    return 'm';
  case KEYCODE_NINE_PRESSED:
    return '9';
  case KEYCODE_N_PRESSED:
    return 'n';
  case KEYCODE_ONE_PRESSED:
    return '1';
  case KEYCODE_O_PRESSED:
    return 'o';
  case KEYCODE_PERIOD_PRESSED:
    return '.';
  case KEYCODE_P_PRESSED:
    return 'p';
  case KEYCODE_Q_PRESSED:
    return 'q';
  case KEYCODE_RIGHT_BRACKET_PRESSED:
    return ']';
  case KEYCODE_R_PRESSED:
    return 'r';
  case KEYCODE_SEMICOLON_PRESSED:
    return ';';
  case KEYCODE_SEVEN_PRESSED:
    return '7';
  case KEYCODE_SINGLE_QUOTE_PRESSED:
    return '\'';
  case KEYCODE_SIX_PRESSED:
    return '6';
  case KEYCODE_SLASH_PRESSED:
    return '/';
  case KEYCODE_SPACE_PRESSED:
    return ' ';
  case KEYCODE_S_PRESSED:
    return 's';
  case KEYCODE_TAB_PRESSED:
    return '\t';
  case KEYCODE_THREE_PRESSED:
    return '3';
  case KEYCODE_TWO_PRESSED:
    return '2';
  case KEYCODE_T_PRESSED:
    return 't';
  case KEYCODE_U_PRESSED:
    return 'u';
  case KEYCODE_V_PRESSED:
    return 'v';
  case KEYCODE_W_PRESSED:
    return 'w';
  case KEYCODE_X_PRESSED:
    return 'x';
  case KEYCODE_Y_PRESSED:
    return 'y';
  case KEYCODE_ZERO_ZERO_PRESSED:
    return '0';
  case KEYCODE_Z_PRESSED:
    return 'z';

  default:
    return '\0';
  }
}
