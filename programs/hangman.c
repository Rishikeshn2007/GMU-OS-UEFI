#include "gmu_api.h"

#include <stddef.h>

static const char *const words[] = {
    "assembly", "computer", "firmware", "keyboard",
    "operating", "processor", "software", "uefi"
};

static void draw_game(
    const GMU_APP_API *api,
    const char *word,
    const char *guessed,
    int misses) {
    int i;
    int found;

    api->clear_screen();
    api->set_color(GMU_COLOR_WHITE_ON_BLACK);
    api->print_string("GMU-OS Hangman\r\n\r\n");
    api->print_string("Word: ");
    for (i = 0; word[i] != '\0'; i++) {
        found = 0;
        for (int j = 0; guessed[j] != '\0'; j++) {
            if (guessed[j] == word[i]) {
                found = 1;
                break;
            }
        }
        api->print_char(found ? word[i] : '_');
        api->print_space();
    }

    api->print_string("\r\n\r\nGuessed letters: ");
    api->print_string(guessed);
    api->print_string("\r\nMisses: ");
    api->print_char((char)('0' + misses));
    api->print_string(" / 6\r\n\r\nType a letter to guess; Esc exits.");
}

int GMU_APP_ABI app_main(const GMU_APP_API *api) {
    unsigned int word_count = sizeof(words) / sizeof(words[0]);

    if (!api || api->abi_version != GMU_APP_ABI_VERSION ||
        api->struct_size < sizeof(GMU_APP_API)) {
        return 1;
    }

    while (1) {
        const char *word = words[api->random(0, word_count - 1)];
        char guessed[27] = "";
        int miss_count = 0;
        int won = 0;

        while (miss_count < 6 && !won) {
            uint8_t key;
            int repeated = 0;
            int correct = 0;
            int complete = 1;
            int guessed_len;

            draw_game(api, word, guessed, miss_count);
            key = api->wait_for_key();
            if (key == GMU_KEY_ESC) return 0;

            if (key >= 'A' && key <= 'Z') key = (uint8_t)(key - 'A' + 'a');
            if (key < 'a' || key > 'z') continue;

            for (int i = 0; guessed[i] != '\0'; i++) {
                if (guessed[i] == (char)key) repeated = 1;
            }
            if (repeated) continue;

            guessed_len = 0;
            while (guessed[guessed_len] != '\0') guessed_len++;
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

        draw_game(api, word, guessed, miss_count);
        api->print_string("\r\n\r\n");
        if (won) {
            api->print_string("You guessed the word!");
        } else {
            api->print_string("Game over. The word was: ");
            api->print_string(word);
        }
        api->print_string("\r\nPress R to play again or Esc to return.");
        while (1) {
            uint8_t key = api->wait_for_key();
            if (key == GMU_KEY_ESC) return 0;
            if (key == 'r' || key == 'R') break;
        }
    }
}
