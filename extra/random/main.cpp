// 番外編「乱数の話」の実験用プログラム
// 画面に点を打って、乱数の「くせ」を目で見る
//
// 操作
//   1   … rand() で、画面のどこか1ピクセルに点を打つ
//   2   … 「掛けて、足して、余りを取る」乱数で、続けて出た2つの数を (x, y) にして点を打つ
//   3   … xorshift（レイトレーサー編 #07 で使ったもの）で、2 と同じことをする
//   4   … シードを 0 にした xorshift で、2 と同じことをする
//   5   … シードを 1 に決めた xorshift で点を打つ。何度押しても、同じ絵になる
//   6   … シードを起動した時刻にした xorshift で点を打つ。押すたびに、絵が変わる
//   Esc … 終わる
//
// 数の並びなどの確かめた結果は、コンソールの窓に出る
// Experiment program for the extra episode "Random Numbers". Plot points on the screen to see patterns in random numbers.
//
// Controls
// 1 ... Use rand() to plot a point at a random pixel.
// 2 ... Use the multiply-add-remainder method to turn two numbers in a row into (x, y) and plot a point.
// 3 ... Use xorshift (from Raytracer #07) to do the same as (2).
// 4 ... Use xorshift with seed 0 to do the same as (2).
// 5 ... Use xorshift with seed 1. The same image appears every time.
// 6 ... Use xorshift seeded with the startup time. The image changes each time.
// Esc ... Exit.
//
// The number sequence and other test results appear in the console window.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "framework/canvas.h"

// 打つ点の数
// The number of points plotted.
const int point_count = 20000;

const Color white = {255, 255, 255};

// 乱数の「今の状態」。呼ぶたびに、次の数へ進める
// 最初の値（シード）が同じなら、そこから先の数は全部同じになる
// The current state of the random number generator. Each call advances it to the next number. Starting with the same seed produces the same sequence.
uint32_t random_state = 1;

// 例1：掛けて、足して、余りを取る。0 〜 65535 の数を返す
// Example 1: multiply, add, and take the remainder. Return a number from 0 to 65535.
uint32_t next_simple() {
    random_state = (random_state * 5 + 1) % 65536;
    return random_state;
}

// 例2：xorshift。レイトレーサー編 #07 の random_float の中身と同じ
// Example 2: xorshift. Same as the random_float implementation in Raytracer #07.
uint32_t next_xorshift() {
    random_state ^= random_state << 13;  // 左に13ずらして重ねる / Offset it 13 units to the left and overlap it.
    random_state ^= random_state >> 17;  // 右に17ずらして重ねる / Offset it 17 units to the right and overlap it.
    random_state ^= random_state << 5;  // 左に5ずらして重ねる / Offset it 5 units to the left and overlap it.
    return random_state;
}

// 0 以上 1 未満のランダムな数を返す。レイトレーサー編 #07 と同じ作り方
// Return a random number from 0 up to, but not including, 1. Made the same way as Raytracer #07.
float random_float() {
    return static_cast<float>(next_xorshift() >> 8) * (1.0f / 16777216.0f);
}

// (x, y) のピクセルを color にする
// Set the pixel at (x, y) to color.
void set_pixel(Canvas& canvas, int x, int y, Color color) {
    // 画面の外は何もしない。はみ出して pixels の外を書くと、プログラムが壊れるため
    // Do nothing outside the screen. Writing past pixels would break the program.
    if (x < 0 || x >= canvas.width || y < 0 || y >= canvas.height) {
        return;
    }
    canvas.pixels[y * canvas.width + x] = color;
}

// 画面を真っ黒にする
// Fill the screen with black.
void clear(Canvas& canvas) {
    for (Color& pixel : canvas.pixels) {
        pixel = Color{0, 0, 0};
    }
}

// 実験1：rand() で、画面のどこか1ピクセルに点を打つ
// Experiment 1: use rand() to plot a point at a random pixel on the screen.
void experiment_rand(Canvas& canvas) {
    clear(canvas);

    const int pixel_count = canvas.width * canvas.height;
    int highest_index = 0;  // 出てきた一番大きな番号 / The largest index found.
    // 上の 6 割（y が 103 より小さい行）に打った点の数
    // The number of points plotted in the top 60% (rows where y is less than 103).
    int upper_count = 0;  
    for (int i = 0; i < point_count; i++) {
        const int index = rand() % pixel_count;  // 画面のどこか1ピクセル / Any pixel on the screen.
        canvas.pixels[index] = white;
        highest_index = std::max(highest_index, index);
        if (index / canvas.width < 103) {
            upper_count++;
        }
    }

    printf("[1] rand(): RAND_MAX = %d, pixels = %d\n", RAND_MAX, pixel_count);
    printf("    highest index = %d (row %d), points in rows 0-102 = %d of %d\n",
           highest_index, highest_index / canvas.width, upper_count, point_count);
}

// 実験2：「掛けて、足して、余りを取る」乱数で、続けて出た2つの数を (x, y) にして点を打つ
// Experiment 2: use the multiply-add-remainder random number method to turn two numbers in a row into (x, y) and plot a point.
void experiment_simple(Canvas& canvas) {
    random_state = 1;
    printf("[2] simple: first numbers =");
    for (int i = 0; i < 8; i++) {
        printf(" %u", next_simple());
    }
    printf("\n");

    clear(canvas);
    random_state = 1;
    for (int i = 0; i < point_count; i++) {
        // 0 以上 1 未満にする
        // Keep the value from 0 up to, but not including, 1.
        const float a = static_cast<float>(next_simple()) / 65536.0f;  
        const float b = static_cast<float>(next_simple()) / 65536.0f;
        set_pixel(canvas, static_cast<int>(a * static_cast<float>(canvas.width)),
                  static_cast<int>(b * static_cast<float>(canvas.height)), white);
    }
}

// xorshift で、続けて出た2つの数を (x, y) にして点を打つ。シードは、呼ぶ前に決めておく
// Use xorshift to turn two numbers in a row into (x, y) and plot a point. Set the seed before calling it.
void draw_xorshift_points(Canvas& canvas) {
    clear(canvas);
    for (int i = 0; i < point_count; i++) {
        const float a = random_float();
        const float b = random_float();
        set_pixel(canvas, static_cast<int>(a * static_cast<float>(canvas.width)),
                  static_cast<int>(b * static_cast<float>(canvas.height)), white);
    }
}

// 実験2の続き：xorshift で、同じことをする
// Continue experiment 2 with xorshift.
void experiment_xorshift(Canvas& canvas) {
    random_state = 1;
    draw_xorshift_points(canvas);
    printf("[3] xorshift, seed = 1\n");
}

// 小ネタ：シードを 0 にすると、いつまでも 0 しか出ない
// Tip: if the seed is 0, it only produces 0 forever.
void experiment_zero_seed(Canvas& canvas) {
    random_state = 0;
    draw_xorshift_points(canvas);
    printf("[4] xorshift, seed = 0: state after drawing = %u\n", random_state);
}

// 実験3：シードを 1 に決めておく。何度押しても、同じ絵になる
// Experiment 3: set the seed to 1. The same image appears every time.
void experiment_fixed_seed(Canvas& canvas) {
    random_state = 1;
    draw_xorshift_points(canvas);
    printf("[5] xorshift, seed = 1 (always the same picture)\n");
}

// 実験3の続き：シードを時刻にする。押すたびに、絵が変わる
// Continue experiment 3 with a time-based seed. The image changes each time you press the key.
void experiment_time_seed(Canvas& canvas) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    random_state = static_cast<uint32_t>(now.count());
    if (random_state == 0) {
        random_state = 1;  // 0 は使えない / 0 is not allowed.
    }
    printf("[6] xorshift, seed = %u (from the clock)\n", random_state);
    draw_xorshift_points(canvas);
}

void run_experiment(Canvas& canvas, int number) {
    switch (number) {
    case 1:
        experiment_rand(canvas);
        break;
    case 2:
        experiment_simple(canvas);
        break;
    case 3:
        experiment_xorshift(canvas);
        break;
    case 4:
        experiment_zero_seed(canvas);
        break;
    case 5:
        experiment_fixed_seed(canvas);
        break;
    case 6:
        experiment_time_seed(canvas);
        break;
    }
}

int main() {
    Canvas canvas(320, 180, 4);  // 320×180 の絵を、4倍に拡大して表示する / Display a 320x180 image enlarged 4 times.

    printf("keys: 1 rand(), 2 simple, 3 xorshift, 4 xorshift seed 0, 5 fixed seed, 6 time seed, Esc quit\n");

    run_experiment(canvas, 1);

    // 前のフレームで、そのキーが押されていたか。押した瞬間だけ実験をやり直すため
    // Whether the key was down in the previous frame. Use this to restart the experiment only when the key is first pressed.
    bool was_down[7] = {};  
    while (canvas.update()) {
        if (canvas.is_key_down(key_escape)) {
            break;
        }

        for (int number = 1; number <= 6; number++) {
            const bool is_down = canvas.is_key_down('0' + number);
            if (is_down && !was_down[number]) {
                run_experiment(canvas, number);
            }
            was_down[number] = is_down;
        }

        canvas.present();
    }
}
