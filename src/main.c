#include <eadk.h>
#include <stdint.h>
#include <stdbool.h>

const char eadk_app_name[]
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_app_name")))
#endif
    = "LED Lamp";

const uint32_t eadk_api_level
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_api_level")))
#endif
    = 0;

/* Appels systeme de la LED, numeros issus de
 * shared/ion/src/device/shared/drivers/svcall.h (Epsilon).
 * L'argument (couleur RGB565) doit etre dans r0 au moment du svc. */
#define SVC_LED_SET_COLOR 37
#define SVC_LED_UPDATE_COLOR_WITH_PLUG_AND_CHARGE 38

static void led_set_color(uint16_t rgb565) {
#if PLATFORM_DEVICE
  register uint32_t r0 __asm__("r0") = rgb565;
  __asm__ volatile("svc %[n]"
                   :
                   : [n] "I"(SVC_LED_SET_COLOR), "r"(r0)
                   : "r1", "r2", "r3", "memory");
#else
  (void)rgb565; /* pas de LED dans le simulateur */
#endif
}

static void led_restore(void) {
#if PLATFORM_DEVICE
  register uint32_t r0 __asm__("r0");
  __asm__ volatile("svc %[n]"
                   : "=r"(r0)
                   : [n] "I"(SVC_LED_UPDATE_COLOR_WITH_PLUG_AND_CHARGE)
                   : "r1", "r2", "r3", "memory");
#endif
}

static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/* Teinte 0..359 -> RGB565 (saturation et valeur max) */
static uint16_t hue_to_color(int h) {
  int region = h / 60;
  int f = (h % 60) * 255 / 60;
  uint8_t up = (uint8_t)f, down = (uint8_t)(255 - f);
  switch (region) {
    case 0: return rgb(255, up, 0);
    case 1: return rgb(down, 255, 0);
    case 2: return rgb(0, 255, up);
    case 3: return rgb(0, down, 255);
    case 4: return rgb(up, 0, 255);
    default: return rgb(255, 0, down);
  }
}

static void draw(uint16_t color, const char* label) {
  eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);
  eadk_display_push_rect_uniform((eadk_rect_t){110, 90, 100, 60}, color);
  eadk_display_draw_string("LED Lamp", (eadk_point_t){110, 10}, true,
                           eadk_color_white, eadk_color_black);
  eadk_display_draw_string(label, (eadk_point_t){10, 170}, false,
                           eadk_color_white, eadk_color_black);
  eadk_display_draw_string("1-7: couleurs  0: eteint", (eadk_point_t){10, 190},
                           false, eadk_color_white, eadk_color_black);
  eadk_display_draw_string("Gauche/Droite: teinte  Retour: quitter",
                           (eadk_point_t){10, 205}, false, eadk_color_white,
                           eadk_color_black);
}

int main(int argc, char* argv[]) {
  int hue = 0;
  uint16_t color = rgb(255, 0, 0);
  const char* label = "Rouge";
  led_set_color(color);
  draw(color, label);

  eadk_keyboard_state_t last = 0;
  while (true) {
    eadk_keyboard_state_t k = eadk_keyboard_scan();
    if (k != last) {
      bool changed = true;
      if (eadk_keyboard_key_down(k, eadk_key_back)) {
        break;
      } else if (eadk_keyboard_key_down(k, eadk_key_one)) {
        color = rgb(255, 0, 0); label = "Rouge";
      } else if (eadk_keyboard_key_down(k, eadk_key_two)) {
        color = rgb(0, 255, 0); label = "Vert";
      } else if (eadk_keyboard_key_down(k, eadk_key_three)) {
        color = rgb(0, 0, 255); label = "Bleu";
      } else if (eadk_keyboard_key_down(k, eadk_key_four)) {
        color = rgb(255, 255, 0); label = "Jaune";
      } else if (eadk_keyboard_key_down(k, eadk_key_five)) {
        color = rgb(0, 255, 255); label = "Cyan";
      } else if (eadk_keyboard_key_down(k, eadk_key_six)) {
        color = rgb(255, 0, 255); label = "Magenta";
      } else if (eadk_keyboard_key_down(k, eadk_key_seven)) {
        color = rgb(255, 255, 255); label = "Blanc";
      } else if (eadk_keyboard_key_down(k, eadk_key_zero)) {
        color = 0; label = "Eteint";
      } else if (eadk_keyboard_key_down(k, eadk_key_right)) {
        hue = (hue + 15) % 360; color = hue_to_color(hue); label = "Teinte";
      } else if (eadk_keyboard_key_down(k, eadk_key_left)) {
        hue = (hue + 345) % 360; color = hue_to_color(hue); label = "Teinte";
      } else {
        changed = false;
      }
      if (changed) {
        led_set_color(color);
        draw(color, label);
      }
    }
    last = k;
    eadk_timing_msleep(20);
  }

  led_restore(); /* rend la LED au firmware (charge / etat normal) */
  return 0;
}
