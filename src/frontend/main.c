#include "utils/yield.h"
#include <stdio.h>

coroutine sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire(void* _arg) {
    yield("do you remember?");
    yield("the 21st night of september");
    yield("love was changing the minds of pretenders");
    yield("while chasing the clouds away");
}

int main() {
    coroutine test = NULL;
    test = call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
}
