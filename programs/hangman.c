#include "apps.h"
#include "../source/features/misc.h"
#include "../source/features/screen.h"
#include "../source/features/keyboard.h"
#include "../source/features/text.h"

static const char *const words[] = {
    "assembly", "computer", "firmware", "keyboard",
    "operating", "processor", "software", "uefi"
};

static void draw_game(const char *word, const char *guessed, int misses) {
    int i;
    int found;

    os_clear_screen();
    os_set_color(WHITE_ON_BLACK);
    os_print_string("MikeOS Hangman\r\n\r\n");
    os_print_string("Word: ");
    for (i = 0; word[i] != '\0'; i++) {
        found = 0;
        for (int j = 0; guessed[j] != '\0'; j++) {
            if (guessed[j] == word[i]) {
                found = 1;
                break;
            }
        }
        os_print_char(found ? word[i] : '_');
        os_print_space();
    }

    os_print_string("\r\n\r\nGuessed letters: ");
    os_print_string(guessed);
    os_print_string("\r\nMisses: ");
    os_print_char((char)('0' + misses));
    os_print_string(" / 6\r\n\r\nType a letter to guess; Esc exits.");
}

void app_hangman(void) {
    unsigned int word_count = sizeof(words) / sizeof(words[0]);

    while (1) {
        const char *word = words[os_get_random(0, word_count - 1)];
        char guessed[27] = "";
        int miss_count = 0;
        int won = 0;

        while (miss_count < 6 && !won) {
            UINT8 key;
            int repeated = 0;
            int correct = 0;
            int complete = 1;
            int guessed_len;

            draw_game(word, guessed, miss_count);
            key = os_wait_for_key();
            if (key == KEY_ESC) return;

            if (key >= 'A' && key <= 'Z') key = (UINT8)(key - 'A' + 'a');
            if (key < 'a' || key > 'z') continue;

            for (int i = 0; guessed[i] != '\0'; i++) {
                if (guessed[i] == (char)key) repeated = 1;
            }
            if (repeated) continue;

            guessed_len = os_string_length(guessed);
            guessed[guessed_len] = (char)key;
            guessed[guessed_len + 1] = '\0';

            for (int i = 0; word[i] != '\0'; i++) {
                int found = 0;
                if (word[i] == (char)key) correct = 1;
                for (int j = 0; guessed[j] != '\0'; j++) {
                    if (guessed[j] == word[i]) found = 1;
                }
                if (!found) complete = 0;
            }

            if (!correct) {
                miss_count++;
            }
            won = complete;
        }

        draw_game(word, guessed, miss_count);
        os_print_string("\r\n\r\n");
        if (won) {
            os_print_string("You guessed the word!");
        } else {
            os_print_string("Game over. The word was: ");
            os_print_string(word);
        }
        os_print_string("\r\nPress R to play again or Esc to return.");
        while (1) {
            UINT8 key = os_wait_for_key();
            if (key == KEY_ESC) return;
            if (key == 'r' || key == 'R') break;
        }
    }
}
