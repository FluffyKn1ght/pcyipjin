#include "utils/yield.h"
#include <stdio.h>

coroutine sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire(void* _arg) {
    int my_demise = 0;
    double consequently_not_my_demise = 26.67;

    yield("do you remember?");
    my_demise++;
    yield("the 21st night of september");
    my_demise++;
    yield("love was changing the minds of pretenders");
    my_demise++;
    yield("while chasing the clouds away");
    printf("%d %f\n", my_demise, consequently_not_my_demise);
    stop();
}

int main() {
    coroutine test = NULL;
    test = coro_call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = coro_call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = coro_call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = coro_call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    printf("%s\n", get_value(char*, test));
    test = coro_call(test, sing_the_classic_song_September_by_the_band_Earth_Wind_and_Fire, NULL);
    coro_free(test);
}
