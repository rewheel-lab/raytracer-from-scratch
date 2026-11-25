// 番外編「ガンマと sRGB」の実験用プログラム
// 画面に出す数字（0〜255）と、光の量のずれを、目で見て確かめる
// Experiment program for the extra episode "Gamma and sRGB". Compare screen values (0 to 255) with light values by eye.
//
// 操作
//   1 … 白黒の細かい縞を、数字の 128 のグレーと、188 のグレーの間に並べる。離れて見て、縞と同じ明るさに見えるのはどちらか
//   2 … 黒から白へのグラデーションを、数字でまっすぐ並べたもの（上）と、光の量でまっすぐ並べたもの（下）で比べる
//   3 … 2色を混ぜたグラデーションを、数字のまま混ぜたもの（上）と、光の量で混ぜたもの（下）で比べる
//   4 … レイトレーサー編 #08 の絵。ガンマ補正（光の量を sRGB の数字に直す）のあり・なしを切り替える
//   5 … 黒から白までを 32 段に分けたグラデーションを、光の量が等しい間隔のもの（上）と、数字が等しい間隔のもの（下）で比べる
//   6 … 白黒の細かい絵を、2×2 のピクセルの平均で半分に縮める。縮める前（左）と、数字のまま平均したもの（中）、光の量で平均したもの（右）を並べる
//   W … （1）縞の幅を 1 ピクセルと 2 ピクセルで切り替える
//   H … （1）188 のグレーを隠す・出す
//   C … （3）混ぜる 2 色の組を切り替える
//   G … （4）ガンマ補正のあり・なしを切り替える
//   B … （4）#08 の確かめ用の帯を出す・隠す
//   R … （4）積み上げを最初からやり直す
//   S … （6）縮める前の絵を切り替える（市松模様 → 細い線の格子 → 縞）
//   P … 今の実験の数字を、コンソールに出す
//   Esc … 終わる
//
// 画面の下の小さな四角の印は、今の実験（左）と、切り替えの今の状態（右）。明るいのが、選ばれている・入っている
// 確かめた数字は、コンソールの窓に出る
// The small squares at the bottom show the current experiment on the left and the current toggle states on the right. Bright squares indicate the selected or enabled options. The measured values are printed to the console.
//
// Controls
// 1 ... Put fine black-and-white stripes next to gray 128 and gray 188. Look from a distance and choose which gray matches the stripes.
// 2 ... Compare a black-to-white gradient that changes evenly as screen values (top) with one that changes evenly as light values (bottom).
// 3 ... Compare two-color gradients mixed as screen values (top) and as light values (bottom).
// 4 ... Show the Raytracer #08 image and toggle gamma correction (convert light values to sRGB values).
// 5 ... Compare 32-step gradients with even light steps (top) and even screen-value steps (bottom).
// 6 ... Shrink a detailed black-and-white image by averaging 2x2 pixel blocks. Compare the original (left), the average of screen values (middle), and the average of light values (right).
// W ... For (1), switch stripe width between 1 and 2 pixels.
// H ... For (1), show or hide gray 188.
// C ... For (3), switch the color pair.
// G ... For (4), toggle gamma correction.
// B ... For (4), show or hide the #08 test bands.
// R ... For (4), restart accumulation.
// S ... For (6), switch the source image (checkerboard -> fine grid -> stripes).
// P ... Print the current experiment values to the console.
// Esc ... Exit.
//
// The small squares at the bottom show the current experiment (left) and toggle states (right). Bright squares are selected or enabled. Test values appear in the console window.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "framework/canvas.h"

// 3D の位置や向き。x が右、y が上、z が奥
// A 3D position or direction. x points right, y points up, and z points away.
struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 operator+(Vec3 a, Vec3 b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator-(Vec3 a, Vec3 b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

// 3つの成分の符号を、まとめてひっくり返す。向きが反対になる
// Flip the signs of all three components. This reverses the direction.
Vec3 operator-(Vec3 v) {
    return Vec3{-v.x, -v.y, -v.z};
}

// 3つの成分を、まとめて s 倍する
// Multiply all three components by s.
Vec3 operator*(Vec3 v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

// 色どうしを掛ける。赤は赤どうし、緑は緑どうし、青は青どうしで、成分ごとに掛ける
// Multiply colors component by component: red by red, green by green, and blue by blue.
Vec3 operator*(Vec3 a, Vec3 b) {
    return Vec3{a.x * b.x, a.y * b.y, a.z * b.z};
}

// 内積。長さ1の向きどうしなら、向きのそろい具合（同じ向きで 1、直角で 0、反対向きで -1）になる
// Dot product. For unit directions, it measures how well they align: 1 for the same direction, 0 for perpendicular, and -1 for opposite directions.
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(Vec3 v) {
    return std::sqrt(dot(v, v));
}

Vec3 normalize(Vec3 v) {
    return v * (1.0f / length(v));  // 長さで割って、長さを1にする / Divide by the length to make the length 1.
}

// d を、法線 n（長さ1）の面で跳ね返した向き
// The direction d reflected by a surface with unit normal n.
Vec3 reflect(Vec3 d, Vec3 n) {
    // 法線の向きの成分だけを逆にする。面に沿った成分はそのまま
    // Flip only the component along the normal. Keep the component along the surface.
    return d - n * (2.0f * dot(d, n));  
}

// 光線。origin から direction の向きに進む
// A ray travels from origin in the direction of direction.
struct Ray {
    Vec3 origin;
    Vec3 direction;  // 長さ1にしておく / Make the length 1.
};

// 画面に出す数字（sRGB。0〜1）を、光の量（リニア。0〜1）に直す。linear_to_srgb の逆向き
// Convert an sRGB screen value from 0 to 1 to a linear light value from 0 to 1. The reverse of linear_to_srgb.
float srgb_to_linear(float s) {
    // 0 のすぐ近くだけは、直線でつなぐ
    // Use a straight line only very close to 0.
    if (s <= 0.04045f) {
        return s / 12.92f;
    }
    return std::pow((s + 0.055f) / 1.055f, 2.4f);
}

// 光の量（リニア。0〜1）を、画面に出す数字（sRGB。0〜1）に直す。#08 と同じ
// 画面は、数字に比例した光り方をしない。暗いところに数字を多めに割り振る決まりがある
// Convert a linear light value from 0 to 1 to an sRGB screen value from 0 to 1. Same as #08. A screen does not emit light in proportion to its values, so more values are assigned to dark shades.
float linear_to_srgb(float x) {
    // 0 のすぐ近くだけは、直線でつなぐ
    // Use a straight line only very close to 0.
    if (x <= 0.0031308f) {
        return x * 12.92f;
    }
    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;  // だいたい「1 / 2.2 乗」 / About "1 / 2.2 power".
}

// 0〜1 の小数を、0〜255 の整数にする
// Convert a decimal value from 0 to 1 to an integer from 0 to 255.
uint8_t to_byte(float x) {
    // 0〜1 の外の値をそのまま 255 倍して uint8_t にすると、値が化けてしまう。先に 0〜1 に切り詰める
    // Converting a value outside 0 to 1 directly to uint8_t after multiplying by 255 can wrap it. Clamp it to 0 to 1 first.
    // + 0.5 で四捨五入する
    // Add 0.5 to round to the nearest integer.
    return static_cast<uint8_t>(std::clamp(x, 0.0f, 1.0f) * 255.0f + 0.5f);  
}

// 画面に出す 0〜255 の数字が、本当は光の量のどれだけか。0〜1
// How much light from 0 to 1 the screen value from 0 to 255 represents.
float byte_to_linear(uint8_t value) {
    return srgb_to_linear(static_cast<float>(value) / 255.0f);
}

// 光の量（リニア。0〜1）を、画面に出す 0〜255 の数字に直す
// Convert a linear light value from 0 to 1 to a screen value from 0 to 255.
uint8_t linear_to_byte(float x) {
    return to_byte(linear_to_srgb(std::clamp(x, 0.0f, 1.0f)));
}

// 0〜1 の色（Vec3 の x, y, z が赤, 緑, 青）を、画面に出す 0〜255 の Color に直す。#08 と同じ
// 計算は全部、光の量のまま進める。画面に出す直前のここで、1回だけ sRGB に直す
// Convert a color from 0 to 1 (Vec3 x, y, z are red, green, blue) to a screen Color from 0 to 255. Same as #08. Keep calculations in light values and convert to sRGB just once here, right before showing the image.
Color to_color(Vec3 c) {
    return Color{linear_to_byte(c.x), linear_to_byte(c.y), linear_to_byte(c.z)};
}

// ガンマ補正をかけない版（#08 の前の to_color）。0〜1 を、そのまま 0〜255 にする
// Without gamma correction (the to_color from before #08). Convert 0 to 1 directly to 0 to 255.
Color to_color_raw(Vec3 c) {
    return Color{to_byte(c.x), to_byte(c.y), to_byte(c.z)};
}

// 赤・緑・青が同じ数字のグレー
// Gray where red, green, and blue have the same value.
Color gray(uint8_t value) {
    return Color{value, value, value};
}

// 色の明るさ。光の量の赤・緑・青を、目の感じ方に合わせた割合で足す（緑がいちばん明るく感じる）
// Color brightness. Add red, green, and blue light values using weights that match human vision; green appears brightest.
float brightness(Color color) {
    return 0.2126f * byte_to_linear(color.r) + 0.7152f * byte_to_linear(color.g) + 0.0722f * byte_to_linear(color.b);
}

// a と b（0〜1。画面の数字）を、数字のまま混ぜる。t が 0 なら a、1 なら b。思い込みの混ぜ方
// Mix a and b (screen values from 0 to 1) as numbers. When t is 0, use a; when t is 1, use b. This is the naive way to mix them.
float mix_numbers(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

// a と b（0〜1。画面の数字）を、光の量で混ぜる。t が 0 なら a、1 なら b
// Mix a and b (screen values from 0 to 1) as light values. When t is 0, use a; when t is 1, use b.
float mix_light(float a, float b, float t) {
    // 混ぜるのは光の量で。sRGB に戻すのは最後に1回
    // Mix light values. Convert back to sRGB only once at the end.
    const float l = srgb_to_linear(a) * (1.0f - t) + srgb_to_linear(b) * t;
    return linear_to_srgb(l);
}

// 色の1つの成分（0〜255）を混ぜる。use_light が true なら光の量で、false なら数字のまま混ぜる
// Mix one color component from 0 to 255. If use_light is true, mix light values; otherwise, mix the numbers as they are.
uint8_t mix_channel(uint8_t a, uint8_t b, float t, bool use_light) {
    const float fa = static_cast<float>(a) / 255.0f;
    const float fb = static_cast<float>(b) / 255.0f;
    return to_byte(use_light ? mix_light(fa, fb, t) : mix_numbers(fa, fb, t));
}

// 2つの色を混ぜる。use_light が true なら光の量で、false なら数字のまま混ぜる
// Mix two colors. If use_light is true, mix light values; otherwise, mix the numbers as they are.
Color mix_color(Color a, Color b, float t, bool use_light) {
    return Color{mix_channel(a.r, b.r, t, use_light), mix_channel(a.g, b.g, t, use_light), mix_channel(a.b, b.b, t, use_light)};
}

// 4つの数字（0〜255）の平均。use_light が true なら光の量で、false なら数字のまま平均する
// Average four values from 0 to 255. If use_light is true, average their light values; otherwise, average the numbers as they are.
uint8_t average_of_four(uint8_t a, uint8_t b, uint8_t c, uint8_t d, bool use_light) {
    if (use_light) {
        const float light = (byte_to_linear(a) + byte_to_linear(b) + byte_to_linear(c) + byte_to_linear(d)) / 4.0f;
        return linear_to_byte(light);
    }
    return static_cast<uint8_t>((a + b + c + d + 2) / 4);  // + 2 で四捨五入する / Round to the nearest integer by adding 0.5.
}

// a と b を混ぜる。t が 0 なら a、1 なら b、その間は割合で混ぜる
// Mix a and b. When t is 0, use a; when t is 1, use b; otherwise, mix them by that amount.
Vec3 mix(Vec3 a, Vec3 b, float t) {
    return a * (1.0f - t) + b * t;
}

// 光線の向きだけで決まる、空の色。#08 と同じ
// The sky color depends only on the ray direction. Same as #08.
Vec3 sky_color(Ray ray) {
    const Vec3 white = {1.0f, 1.0f, 1.0f};
    const Vec3 sky_blue = {0.5f, 0.7f, 1.0f};

    const float t = 0.5f * (ray.direction.y + 1.0f);  // 下向き 0 〜 上向き 1 / Down is 0; up is 1.
    return mix(white, sky_blue, t);  // t が大きいほど青 / A larger t is more blue.
}

// 球。中心と半径と色で表す
// Sphere. Defined by its center, radius, and color.
struct Sphere {
    Vec3 center;
    float radius;
    // 0〜1 の色。光を、赤・緑・青のそれぞれ何割返すか（アルベド）。鏡の球では使わない
    // A color from 0 to 1. It sets how much red, green, and blue light is returned (albedo). Not used by mirror spheres.
    Vec3 color;  
    bool mirror = false;  // true なら、光を跳ね返す鏡になる / When true, this becomes a mirror that reflects light.
};

// 光線が球に当たったら true を返し、当たった距離を t に入れる。外れたら t は触らない
// 光線の上の点は origin + direction * t。この t が「光線を何歩進んだか」になる
// t_min 以上 t_max 以下の範囲にある解だけを、当たりとみなす
// Return true if a ray hits a sphere and store the hit distance in t. If it misses, leave t unchanged. A point on the ray is origin + direction * t; t is how far the ray travels. Only count solutions from t_min to t_max as hits.
bool hit_sphere(Ray ray, Sphere sphere, float t_min, float t_max, float& t) {
    // 光線の上の点が球の表面にある条件は、t の2次方程式 a t^2 + b t + c = 0 になる
    // A point on the ray is on the sphere when t solves the quadratic equation a t^2 + b t + c = 0.
    const Vec3 oc = ray.origin - sphere.center;
    const float a = dot(ray.direction, ray.direction);
    const float b = 2.0f * dot(oc, ray.direction);
    const float c = dot(oc, oc) - sphere.radius * sphere.radius;

    // 判別式がマイナスなら、光線のどこも球に届かない
    // If the discriminant is negative, the ray never reaches the sphere.
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    // 解は2つ（球に入る点と出る点）。見えるのは手前なので、まず t が小さいほうを調べる
    // There are two solutions: the entry and exit points on the sphere. The nearer one is visible, so check the smaller t first.
    const float root = std::sqrt(discriminant);
    float t_hit = (-b - root) / (2.0f * a);

    // 手前の解が範囲の外なら、奥の解を調べる。光線が球の中から出発することもあるため
    // If the nearer solution is outside the range, check the farther one. A ray can start inside a sphere.
    if (t_hit < t_min || t_hit > t_max) {
        t_hit = (-b + root) / (2.0f * a);
        if (t_hit < t_min || t_hit > t_max) {
            return false;
        }
    }

    t = t_hit;
    return true;
}

// 球は全部ここに並べる。#08 の絵から、合わせ鏡の壁を除いたもの
// All spheres are listed here. This is the #08 image without the parallel mirror walls.
const int sphere_count = 4;
const Sphere spheres[] = {
    {Vec3{-1.9f, 0.0f, 3.8f}, 1.0f, Vec3{0.9f, 0.1f, 0.1f}},
    // 真ん中の鏡の球。鏡なので、色は使わない
    // The center mirror sphere. It is a mirror, so no color is used.
    {Vec3{0.0f, 0.0f, 5.0f}, 1.0f, Vec3{0.2f, 0.8f, 0.3f}, true},  
    {Vec3{1.4f, 0.0f, 7.5f}, 1.0f, Vec3{0.1f, 0.8f, 0.2f}},
    // 半径1000の球。大きすぎて、近くは平らに見える。上の端が y = -1 になる位置に置く
    // A sphere with radius 1000. It is so large that it looks flat nearby. Place it so its top edge is at y = -1.
    {Vec3{0.0f, -1001.0f, 0.0f}, 1000.0f, Vec3{0.8f, 0.8f, 0.75f}},
};

// 光線が最初に当たったものの情報
// Information about the first thing a ray hits.
struct Hit {
    float t;  // 光線の始点から、当たった点までの距離 / Distance from the ray origin to the hit point.
    int index;  // 当たった球の番号。spheres の何番目か / The index of the hit sphere in spheres.
    Vec3 point;  // 当たった点 / The hit point.
    Vec3 normal;  // 当たった点の法線（長さ1） / The unit normal at the hit point.
};

// 全部の球を調べて、一番手前に当たったものを hit に入れる。何にも当たらなければ false
// t_min より近い当たりは無視する
// Check all spheres and store the closest hit in hit. Return false if nothing is hit. Ignore hits closer than t_min.
bool hit_scene(Ray ray, float t_min, Hit& hit) {
    // これまでで一番近い当たりの距離。最初は「とても遠く」にしておく
    // The closest hit so far. Start with a very large distance.
    float closest = 1e30f;  
    int closest_index = -1;

    for (int i = 0; i < sphere_count; i++) {
        float t = 0.0f;
        // closest より遠いものは範囲の外になる。だから、当たったら必ずこれまでより近い
        // Anything farther than closest is outside the range. So any hit is closer than the previous one.
        if (hit_sphere(ray, spheres[i], t_min, closest, t)) {
            closest = t;
            closest_index = i;
        }
    }

    if (closest_index < 0) {
        return false;
    }

    hit.t = closest;
    hit.index = closest_index;
    hit.point = ray.origin + ray.direction * closest;
    // 法線は、中心から当たった点へ向かう向き。半径で割って長さを1にする
    // The normal points from the center to the hit point. Divide by the radius to make it unit length.
    hit.normal = (hit.point - spheres[closest_index].center) * (1.0f / spheres[closest_index].radius);
    return true;
}

// 乱数の「今の状態」。呼ぶたびに、次の数へ進める。0 にしてはいけない（0 からは、いつまでも 0 しか出ない）
// The current state of the random number generator. It advances on each call. Do not set it to 0, or it will only produce 0 forever.
uint32_t random_state = 1;

// 0 以上 1 未満のランダムな数を返す。決まった計算で、バラバラに見える数を作っている（中身は番外編「乱数の話」で）
// Return a random number from 0 up to, but not including, 1. A fixed calculation makes numbers that look random (see the extra episode "Random Numbers").
float random_float() {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    // 上の24ビットだけを使う。float が過不足なく表せる桁数なので、1.0 にはならない
    // Use only the upper 24 bits. float has enough precision for this, so the value does not become 1.0.
    return static_cast<float>(random_state >> 8) * (1.0f / 16777216.0f);
}

// ランダムな向き（長さ1）を返す。どの向きも、同じ確率で出る
// Return a random unit direction. Every direction has the same chance.
Vec3 random_direction() {
    while (true) {
        // -1 〜 1 の立方体の中に、ランダムな点を取る
        // Pick a random point inside the cube from -1 to 1.
        const Vec3 p = {random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f};
        const float length_squared = dot(p, p);

        // 球の中に入った点だけを使う。立方体のままだと、角の向きが出やすくなって、向きが偏る
        // 中心に近すぎる点も捨てる。長さが 0 に近いと、長さ1にそろえられない
        // Use only points inside the sphere. Sampling the whole cube favors corner directions and makes the distribution uneven. Also reject points too close to the center, since a near-zero length cannot be normalized.
        if (length_squared <= 1.0f && length_squared > 0.0001f) {
            return normalize(p);  // 長さを1にして、向きだけにする / Normalize it to length 1 so only its direction remains.
        }
    }
}

// 鏡に当たるたびに、明るさが何倍になるか。本物の鏡も、光を100% は跳ね返さない
// The brightness multiplier for each mirror bounce. A real mirror does not reflect 100% of the light either.
const float mirror_reflectance = 0.9f;

// 跳ね返る回数の上限。鏡に映った鏡に映った……や、ざらざらの面で跳ね返り続ける光線が、いつまでも終わらないようにする
// The maximum number of bounces, so rays cannot continue forever between mirrors or bounce forever on rough surfaces.
const int max_depth = 20;

// 光線が運んでくる色を求める。depth は、ここまでに何回跳ね返ったか
// Find the color carried by a ray. depth is the number of bounces so far.
Vec3 trace(Ray ray, int depth) {
    // 自分を呼ぶ関数は、まず「どこで止まるか」を書く。上限まで跳ね返ったら、そこで打ち切る
    // For a recursive function, first define where it stops. Stop after reaching the bounce limit.
    if (depth >= max_depth) {
        return Vec3{0.0f, 0.0f, 0.0f};
    }

    // 当たらなければ空の色
    // If there is no hit, use the sky color.
    Hit hit = {};
    if (!hit_scene(ray, 0.001f, hit)) {
        return sky_color(ray);  // 空へ抜けた光線は、空の光を持ち帰る / A ray that escapes into the sky carries back the sky light.
    }

    if (spheres[hit.index].mirror) {
        // 鏡なら、跳ね返った光線で、もう一度 trace を呼ぶ（関数が自分自身を呼ぶ。再帰）
        // 出発点から 0.001 より近い当たりは無視しているおかげで、跳ね返った光線は自分の表面に当たらない
        // For a mirror, trace the reflected ray again (the function calls itself recursively). Ignoring hits closer than 0.001 to the ray origin keeps the bounced ray from hitting its own surface.
        const Ray reflected = {hit.point, reflect(ray.direction, hit.normal)};
        return trace(reflected, depth + 1) * mirror_reflectance;
    }

    // ざらざらの面（拡散反射）は、光をいろいろな向きに跳ね返す。どこへ跳ねるか分からないので、向きはランダムに1つ選ぶ
    // Rough surfaces (diffuse reflection) send light in many directions. Since we do not know which way it goes, choose a random direction.
    Vec3 direction = random_direction();

    // 面の内側を向いたら、ひっくり返す。内側に向かった光線は、面の中にもぐってしまう
    // If it points into the surface, flip it. A ray pointing inward would go below the surface.
    if (dot(direction, hit.normal) < 0.0f) {
        direction = -direction;
    }

    // 跳ね返った先の光を、もう一度 trace で調べる。持ち帰った光に、この面の色を掛ける
    // 赤い面なら、赤い光を多めに返す
    // Trace the bounced ray again, then multiply its returned light by this surface color. A red surface returns more red light.
    const Ray bounced = {hit.point, direction};
    return spheres[hit.index].color * trace(bounced, depth + 1);
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

// 四角形（x, y から幅 w、高さ h）を color で塗る
// Fill a rectangle from (x, y), with width w and height h, using color.
void fill_rect(Canvas& canvas, int x, int y, int w, int h, Color color) {
    for (int v = 0; v < h; v++) {
        for (int u = 0; u < w; u++) {
            set_pixel(canvas, x + u, y + v, color);
        }
    }
}

// 画面全体を、実験の背景の色にする。暗めのグレー
// Fill the screen with a dark gray experiment background.
void clear(Canvas& canvas) {
    fill_rect(canvas, 0, 0, canvas.width, canvas.height, gray(24));
}

// 確かめ用の帯。画面の左上に、同じ大きさの帯を2本、横に並べる。#08 と同じ
//   左：白と黒を1ピクセルずつ交互に並べた縞。光の量は、ちょうど白の半分になる
//   右：光の量 0.5 のグレー
// 帯の外は、color をそのまま返す
// Test bands: place two equal-sized horizontal bands side by side in the top-left corner, same as #08. Left: alternating white and black pixels, with half the light value of white. Right: gray with light value 0.5. Outside the bands, return color unchanged.
Vec3 gray_bands(int x, int y, Vec3 color) {
    const int left = 8;
    const int top = 8;
    const int band_width = 48;
    const int band_height = 24;

    if (y < top || y >= top + band_height) {
        return color;
    }

    if (x >= left && x < left + band_width) {
        if (x % 2 == 0) {
            return Vec3{1.0f, 1.0f, 1.0f};
        }
        return Vec3{0.0f, 0.0f, 0.0f};
    }

    if (x >= left + band_width && x < left + band_width * 2) {
        return Vec3{0.5f, 0.5f, 0.5f};
    }

    return color;
}

// キーで切り替える設定
// A setting toggled by a key.
struct Options {
    int stripe_width;  // 実験1：縞の幅（ピクセル）。1 か 2 / Experiment 1: stripe width in pixels, either 1 or 2.
    bool show_188;  // 実験1：188 のグレーを出すか / Experiment 1: whether to show the gray value 188.
    int color_pair;  // 実験3：混ぜる 2 色の組。color_pairs の何番目か / Experiment 3: which pair of colors to mix in color_pairs.
    bool gamma;  // 実験4：ガンマ補正をかけるか / Experiment 4: whether to apply gamma correction.
    bool bands;  // 実験4：確かめ用の帯を出すか / Experiment 4: whether to show the test bands.
    // 実験6：縮める前の絵。0 が市松模様、1 が細い線の格子、2 が縞
    // Experiment 6: the image before shrinking. 0 is a checkerboard, 1 is a fine grid, and 2 is stripes.
    int source_pattern;  
};

// 実験3で混ぜる 2 色の組
// The pair of colors to mix in experiment 3.
struct ColorPair {
    Color a;  // 左端の色 / The color at the left edge.
    Color b;  // 右端の色 / The color at the right edge.
    const char* name;
};

const int color_pair_count = 3;
const ColorPair color_pairs[] = {
    {Color{255, 0, 0}, Color{0, 255, 0}, "red - green"},
    {Color{255, 0, 0}, Color{0, 0, 255}, "red - blue"},
    {Color{0, 0, 255}, Color{255, 255, 0}, "blue - yellow"},
};

// 四角形の中の平均。グレーの絵に使うので、赤の成分だけを見る
// The average inside the square. For the grayscale image, use only the red component.
struct Average {
    float number;  // 画面に出している数字（0〜255）の平均 / The average screen value from 0 to 255.
    float light;  // 光の量（0〜1）の平均 / The average light value from 0 to 1.
};

Average measure(const Canvas& canvas, int x, int y, int w, int h) {
    // 何千ピクセルも足すので、float だと誤差が目に見える大きさになる。足し算には double を使う
    // Thousands of pixels are added, so float error becomes visible. Use double for the sum.
    double number_sum = 0.0;
    double light_sum = 0.0;
    for (int v = 0; v < h; v++) {
        for (int u = 0; u < w; u++) {
            const uint8_t value = canvas.pixels[(y + v) * canvas.width + (x + u)].r;
            number_sum += static_cast<double>(value);
            light_sum += static_cast<double>(byte_to_linear(value));
        }
    }
    const double count = static_cast<double>(w * h);
    return Average{static_cast<float>(number_sum / count), static_cast<float>(light_sum / count)};
}

// 実験1：縞と、128 のグレーと、188 のグレー
// Experiment 1: the stripes, gray 128, and gray 188.
const int band_width = 100;
const int band_top = 14;
const int band_height = 140;

void experiment_stripes(Canvas& canvas, int stripe_width, bool show_188) {
    clear(canvas);

    // 左から、188 のグレー、縞、128 のグレー。縞を真ん中に置いて、どちらのグレーとも隣り合わせにする
    // From left to right: gray 188, stripes, and gray 128. Put the stripes in the middle next to both grays.
    const int band_count = show_188 ? 3 : 2;
    const int left = (canvas.width - band_width * band_count) / 2;
    const int stripes_x = show_188 ? left + band_width : left;
    const int gray_128_x = stripes_x + band_width;

    if (show_188) {
        fill_rect(canvas, left, band_top, band_width, band_height, gray(188));
    }
    fill_rect(canvas, gray_128_x, band_top, band_width, band_height, gray(128));

    // 縞は縦に並べる。白から始めて、stripe_width 列ごとに白と黒を入れ替える
    // Draw vertical stripes. Start with white and switch between white and black every stripe_width columns.
    for (int u = 0; u < band_width; u++) {
        const bool is_white = (u / stripe_width) % 2 == 0;
        fill_rect(canvas, stripes_x + u, band_top, 1, band_height, is_white ? gray(255) : gray(0));
    }

    // 画面に出ているピクセルから、縞の平均を測る
    // Measure the average stripe value from the displayed pixels.
    const Average stripes = measure(canvas, stripes_x, band_top, band_width, band_height);
    std::printf("[1] stripes (width %d px) next to gray 128%s\n", stripe_width, show_188 ? " and gray 188" : "");
    std::printf("    stripes are 255 and 0: average of the numbers = %.1f, average of the light = %.4f (as a number: %d)\n",
                static_cast<double>(stripes.number), static_cast<double>(stripes.light), linear_to_byte(stripes.light));
    std::printf("    gray 128: light = %.4f\n", static_cast<double>(byte_to_linear(128)));
    if (show_188) {
        std::printf("    gray 188: light = %.4f\n", static_cast<double>(byte_to_linear(188)));
    }
}

// 実験2・3・5で使う、横長の帯の場所。上と下に1本ずつ
// The positions of the two horizontal bands used in experiments 2, 3, and 5.
const int row_left = 32;
// 257 本の列。t = 0/256 〜 256/256 になり、真ん中の列がちょうど 0.5 になる
// 257 columns. t goes from 0/256 to 256/256, so the middle column is exactly 0.5.
const int row_width = 257;  
const int row_height = 56;
const int top_row_y = 30;
const int bottom_row_y = 98;

// 上の帯と下の帯の間に、真ん中（t = 0.5）の目印を引く
// Draw a marker in the middle (t = 0.5) between the upper and lower bands.
void draw_center_tick(Canvas& canvas) {
    fill_rect(canvas, row_left + 128, top_row_y + row_height, 1, bottom_row_y - top_row_y - row_height, Color{96, 160, 255});
}

// 実験2：黒から白へのグラデーション
//   上：左端 0、右端 255 の数字を、まっすぐ（数字に比例して）並べる
//   下：左端 0、右端 1 の光の量を、まっすぐ（光の量に比例して）並べて、最後に sRGB の数字に直す
// Experiment 2: a black-to-white gradient. Top: values from 0 at the left to 255 at the right, changing evenly as numbers. Bottom: light values from 0 to 1, changing evenly as light, then converted to sRGB values.
void experiment_gradient(Canvas& canvas) {
    clear(canvas);

    for (int i = 0; i < row_width; i++) {
        const float t = static_cast<float>(i) / 256.0f;
        fill_rect(canvas, row_left + i, top_row_y, 1, row_height, gray(to_byte(t)));
        fill_rect(canvas, row_left + i, bottom_row_y, 1, row_height, gray(linear_to_byte(t)));
    }
    draw_center_tick(canvas);

    std::printf("[2] gradient from black to white (top: numbers straight, bottom: light straight)\n");
    std::printf("    position  top: number (light)    bottom: number (light)\n");
    const float positions[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    for (const float position : positions) {
        const int column = row_left + static_cast<int>(position * 256.0f + 0.5f);
        const uint8_t top = canvas.pixels[(top_row_y + 1) * canvas.width + column].r;
        const uint8_t bottom = canvas.pixels[(bottom_row_y + 1) * canvas.width + column].r;
        std::printf("    %.2f      %3d (%.4f)          %3d (%.4f)\n", static_cast<double>(position), top,
                    static_cast<double>(byte_to_linear(top)), bottom, static_cast<double>(byte_to_linear(bottom)));
    }
}

// 色の数字を、"(r, g, b)" の形でコンソールに出す
// Print the color values to the console as "(r, g, b)".
void print_color(const char* label, Color color) {
    std::printf("%s(%d, %d, %d)", label, color.r, color.g, color.b);
}

// 実験3：2色を混ぜたグラデーション
//   上：左端と右端の色を、数字のまま混ぜる
//   下：光の量に直してから混ぜて、最後に sRGB の数字に戻す
// Experiment 3: a gradient that mixes two colors. Top: mix the left and right colors as numbers. Bottom: convert to light values, mix, then convert back to sRGB values.
void experiment_mix(Canvas& canvas, const ColorPair& pair) {
    clear(canvas);

    for (int i = 0; i < row_width; i++) {
        const float t = static_cast<float>(i) / 256.0f;
        fill_rect(canvas, row_left + i, top_row_y, 1, row_height, mix_color(pair.a, pair.b, t, false));
        fill_rect(canvas, row_left + i, bottom_row_y, 1, row_height, mix_color(pair.a, pair.b, t, true));
    }
    draw_center_tick(canvas);

    // 真ん中の列は、ちょうど半分ずつ混ぜた色
    // The middle column mixes the colors exactly halfway.
    const Color top_middle = canvas.pixels[(top_row_y + 1) * canvas.width + row_left + 128];
    const Color bottom_middle = canvas.pixels[(bottom_row_y + 1) * canvas.width + row_left + 128];
    std::printf("[3] mix %s (top: numbers, bottom: light)\n", pair.name);
    std::printf("    middle, top    = ");
    print_color("", top_middle);
    std::printf(": brightness (light) %.4f\n", static_cast<double>(brightness(top_middle)));
    std::printf("    middle, bottom = ");
    print_color("", bottom_middle);
    std::printf(": brightness (light) %.4f\n", static_cast<double>(brightness(bottom_middle)));
    std::printf("    both ends: brightness %.4f and %.4f (the average is %.4f)\n", static_cast<double>(brightness(pair.a)),
                static_cast<double>(brightness(pair.b)), static_cast<double>((brightness(pair.a) + brightness(pair.b)) * 0.5f));

    // 帯の中で一番暗い列。数字のまま混ぜると、両端より暗いところができる
    // The darkest column in the band. Mixing screen values directly can make a point darker than either end.
    for (int row = 0; row < 2; row++) {
        const int y = (row == 0) ? top_row_y : bottom_row_y;
        float darkest = 1e30f;
        int darkest_column = 0;
        for (int i = 0; i < row_width; i++) {
            const float b = brightness(canvas.pixels[(y + 1) * canvas.width + row_left + i]);
            if (b < darkest) {
                darkest = b;
                darkest_column = i;
            }
        }
        std::printf("    %s: darkest column = %d of 256 (brightness %.4f)\n", (row == 0) ? "top   " : "bottom", darkest_column,
                    static_cast<double>(darkest));
    }
}

// 実験5：黒から白までを 32 段に分けたグラデーション。1段は 8 列
//   上：光の量を 32 等分する（光の量が等しい間隔）。段の光の量は k / 31
//   下：数字を 32 等分する（数字が等しい間隔）。段の数字は 255 * k / 31
// Experiment 5: a gradient from black to white in 32 steps, 8 columns per step. Top divides light values evenly; each step uses k / 31. Bottom divides screen values evenly; each step uses 255 * k / 31.
void experiment_steps(Canvas& canvas) {
    clear(canvas);

    const int step_count = 32;
    uint8_t top_values[step_count] = {};
    uint8_t bottom_values[step_count] = {};
    for (int k = 0; k < step_count; k++) {
        const float t = static_cast<float>(k) / 31.0f;
        top_values[k] = linear_to_byte(t);
        bottom_values[k] = to_byte(t);
        fill_rect(canvas, row_left + k * 8, top_row_y, 8, row_height, gray(top_values[k]));
        fill_rect(canvas, row_left + k * 8, bottom_row_y, 8, row_height, gray(bottom_values[k]));
    }

    std::printf("[5] 32 steps from black to white (top: even in light, bottom: even in numbers)\n");
    const char* labels[] = {"top   ", "bottom"};
    const uint8_t* rows[] = {top_values, bottom_values};
    for (int row = 0; row < 2; row++) {
        std::printf("    %s:", labels[row]);
        int smallest_gap = 255;
        int largest_gap = 0;
        for (int k = 0; k < step_count; k++) {
            std::printf(" %d", rows[row][k]);
            if (k > 0) {
                const int gap = rows[row][k] - rows[row][k - 1];
                smallest_gap = std::min(smallest_gap, gap);
                largest_gap = std::max(largest_gap, gap);
            }
        }
        std::printf("\n    %s: step between neighbors, smallest %d, largest %d (first step %d, last step %d)\n", labels[row],
                    smallest_gap, largest_gap, rows[row][1] - rows[row][0], rows[row][31] - rows[row][30]);
    }
}

// 実験6：縮める前の絵の (x, y) の数字。0 か 255
//   pattern 0 … 1ピクセルずつ白と黒が入れ替わる市松模様
//   pattern 1 … 8ピクセルおきに、1ピクセルの白い線を引いた格子
//   pattern 2 … 1ピクセルずつ白と黒が入れ替わる縦縞（実験1の縞と同じ）
// Experiment 6: the screen value at (x, y) in the image before shrinking, either 0 or 255. pattern 0 is a checkerboard that switches between white and black every pixel. pattern 1 is a grid with a 1-pixel white line every 8 pixels. pattern 2 is vertical stripes that switch every pixel, like experiment 1.
uint8_t source_value(int pattern, int x, int y) {
    if (pattern == 0) {
        return ((x + y) % 2 == 0) ? 255 : 0;
    }
    if (pattern == 1) {
        return (x % 8 == 0 || y % 8 == 0) ? 255 : 0;
    }
    return (x % 2 == 0) ? 255 : 0;
}

const int panel_width = 96;
const int panel_height = 128;
const int panel_top = 16;
const int panel_gap = 6;

// 実験6：縮める前の絵（左）を、2×2 のピクセルの平均で半分に縮める
//   中：数字のまま平均する
//   右：光の量で平均してから、sRGB の数字に戻す
// 縮めた絵は、縮める前と同じ大きさになるように、1ピクセルを 2×2 に拡大して並べる
// Experiment 6: shrink the image on the left to half size by averaging each 2x2 pixel block. The middle uses screen values; the right averages light values and converts back to sRGB. Enlarge each reduced pixel to 2x2 so the result matches the original size.
void experiment_shrink(Canvas& canvas, int pattern) {
    clear(canvas);

    const int left = (canvas.width - (panel_width * 3 + panel_gap * 2)) / 2;
    const int step = panel_width + panel_gap;

    for (int y = 0; y < panel_height; y++) {
        for (int x = 0; x < panel_width; x++) {
            set_pixel(canvas, left + x, panel_top + y, gray(source_value(pattern, x, y)));
        }
    }

    for (int panel = 1; panel <= 2; panel++) {
        const bool use_light = (panel == 2);
        for (int by = 0; by < panel_height / 2; by++) {
            for (int bx = 0; bx < panel_width / 2; bx++) {
                const uint8_t a = source_value(pattern, bx * 2, by * 2);
                const uint8_t b = source_value(pattern, bx * 2 + 1, by * 2);
                const uint8_t c = source_value(pattern, bx * 2, by * 2 + 1);
                const uint8_t d = source_value(pattern, bx * 2 + 1, by * 2 + 1);
                fill_rect(canvas, left + step * panel + bx * 2, panel_top + by * 2, 2, 2, gray(average_of_four(a, b, c, d, use_light)));
            }
        }
    }

    // 3つの絵の、光の量の平均。縮めても、絵の明るさは変わらないはず
    // The average light value of the three images. The image should stay the same brightness after shrinking.
    const char* pattern_names[] = {"checkerboard", "thin line grid", "stripes"};
    std::printf("[6] shrink by averaging 2x2 pixels (source: %s)\n", pattern_names[pattern]);
    const char* labels[] = {"source              ", "shrunk, numbers     ", "shrunk, light       "};
    for (int panel = 0; panel < 3; panel++) {
        const Average average = measure(canvas, left + step * panel, panel_top, panel_width, panel_height);
        std::printf("    %s: average of the numbers = %.1f, average of the light = %.4f (as a number: %d)\n", labels[panel],
                    static_cast<double>(average.number), static_cast<double>(average.light), linear_to_byte(average.light));
    }
}

// 実験4：(x, y) から 8×8 の範囲の、平均の色をコンソールに出す。ノイズが残っていても読めるように、ならして見る
// Experiment 4: print the average color in an 8x8 area starting at (x, y). Averaging makes the result easier to read even with noise.
void print_block(const Canvas& canvas, const char* label, int x, int y) {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    for (int v = 0; v < 8; v++) {
        for (int u = 0; u < 8; u++) {
            const Color color = canvas.pixels[(y + v) * canvas.width + (x + u)];
            r += static_cast<float>(color.r);
            g += static_cast<float>(color.g);
            b += static_cast<float>(color.b);
        }
    }
    std::printf("    %s (8x8 block at %d, %d): (%.0f, %.0f, %.0f)\n", label, x, y, static_cast<double>(r / 64.0f),
                static_cast<double>(g / 64.0f), static_cast<double>(b / 64.0f));
}

// 実験4：今の絵の数字をコンソールに出す
// Experiment 4: print the current image values to the console.
void print_raytracer(const Canvas& canvas, bool gamma, bool bands, int sample_count) {
    int sum = 0;  // 全部のピクセルの、赤・緑・青の数字の合計 / The total red, green, and blue values for all pixels.
    for (const Color& pixel : canvas.pixels) {
        sum += pixel.r + pixel.g + pixel.b;
    }
    std::printf("[4] raytracer picture, gamma correction: %s, samples = %d\n", gamma ? "on" : "off", sample_count);
    std::printf("    average of all the numbers = %.1f\n", static_cast<double>(sum) / static_cast<double>(canvas.pixels.size() * 3));
    print_block(canvas, "sky   ", 152, 4);
    print_block(canvas, "ground", 248, 112);
    print_block(canvas, "sphere", 76, 86);
    if (bands) {
        // 帯の右側は、光の量 0.5 のグレー
        // The right side of the band is gray with light value 0.5.
        const Color color = canvas.pixels[20 * canvas.width + 80];
        std::printf("    right band (light 0.5) = %d\n", color.r);
    }
}

// 実験4：1回分（1フレーム分）の光線を飛ばして、合計に足し、平均を画面に出す。#08 の main の中身と同じ
// sample_count は、今回の分を足した、足した回数
// Experiment 4: trace one sample (one frame), add it to the sum, and show the average. Same as main in #08. sample_count is the number of samples added, including this one.
void accumulate_frame(Canvas& canvas, std::vector<Vec3>& sum, int sample_count, bool gamma, bool bands) {
    const float width = static_cast<float>(canvas.width);
    const float height = static_cast<float>(canvas.height);
    const float aspect = width / height;  // 画面の縦横比。320 ÷ 180 / The screen aspect ratio: 320 / 180.

    for (int y = 0; y < canvas.height; y++) {
        for (int x = 0; x < canvas.width; x++) {
            // ピクセルの中のランダムな位置を通る光線を作る。カメラは原点で、+Z を向く
            // Make a ray through a random position inside the pixel. The camera is at the origin and faces +Z.
            const float px = static_cast<float>(x) + random_float();
            const float py = static_cast<float>(y) + random_float();

            // スクリーンは z = 1 の位置に置く。横幅が -1 〜 1 になる
            // Place the screen at z = 1, with a width from -1 to 1.
            const float u = px / width * 2.0f - 1.0f;

            // 画面の y は下向きだが、世界の +Y は上向き。だから符号をひっくり返す
            // 縦は横より短いので、縦横比で割って、-1 〜 1 より狭くする。割らないと横に引き伸ばされる
            // Screen y points down, but world +Y points up, so flip the sign. The screen is shorter than it is wide, so divide by the aspect ratio to keep it narrower than -1 to 1; otherwise it stretches horizontally.
            const float v = (1.0f - py / height * 2.0f) / aspect;

            const Ray ray = {Vec3{0.0f, 0.0f, 0.0f}, normalize(Vec3{u, v, 1.0f})};

            // 今回の1回分を、合計に足す
            // Add this sample to the sum.
            const int i = y * canvas.width + x;
            sum[i] = sum[i] + trace(ray, 0);

            // 表示するのは、合計ではなく平均。合計を、足した回数で割る
            // Show the average, not the sum. Divide the sum by the sample count.
            Vec3 average = sum[i] * (1.0f / static_cast<float>(sample_count));
            if (bands) {
                average = gray_bands(x, y, average);
            }

            // 平均はリニアのまま。ガンマ補正をかけるかどうかは、画面に出す最後の1行だけで決まる
            // Keep the average in linear light values. Gamma correction is applied only on the final line that displays the image.
            set_pixel(canvas, x, y, gamma ? to_color(average) : to_color_raw(average));
        }
    }
}

// 小さな四角の印を、右下に並べる。明るいのが、選ばれている・入っているもの
// Arrange small square markers at the bottom-right. The bright ones are selected or enabled.
void draw_option_marks(Canvas& canvas, const bool on[], int count) {
    for (int k = 0; k < count; k++) {
        const Color color = on[k] ? Color{240, 240, 240} : Color{64, 64, 90};
        fill_rect(canvas, canvas.width - 10 - (count - 1 - k) * 10, canvas.height - 6, 8, 4, color);
    }
}

// 今の実験と、切り替えの状態を、画面の下の印で示す
// Show the current experiment and toggle states with the markers at the bottom of the screen.
void draw_marks(Canvas& canvas, int experiment, const Options& options) {
    for (int k = 1; k <= 6; k++) {
        const Color color = (k == experiment) ? Color{240, 240, 240} : Color{64, 64, 90};
        fill_rect(canvas, 2 + (k - 1) * 10, canvas.height - 6, 8, 4, color);
    }

    switch (experiment) {
    case 1: {
        const bool on[] = {options.stripe_width == 2, options.show_188};
        draw_option_marks(canvas, on, 2);
        break;
    }
    case 3: {
        const bool on[] = {options.color_pair == 0, options.color_pair == 1, options.color_pair == 2};
        draw_option_marks(canvas, on, 3);
        break;
    }
    case 4: {
        const bool on[] = {options.gamma, options.bands};
        draw_option_marks(canvas, on, 2);
        break;
    }
    case 6: {
        const bool on[] = {options.source_pattern == 0, options.source_pattern == 1, options.source_pattern == 2};
        draw_option_marks(canvas, on, 3);
        break;
    }
    default:
        break;
    }
}

// キーが押された瞬間だけ true を返す。押しっぱなしでは1回しか反応しない
// Return true only on the frame when the key is pressed. Holding it down does not trigger it again.
bool key_pressed(Canvas& canvas, int key, bool was_down[]) {
    const bool is_down = canvas.is_key_down(key);
    const bool pressed = is_down && !was_down[key];
    was_down[key] = is_down;
    return pressed;
}

int main() {
    Canvas canvas(320, 180, 4);  // 320×180 の絵を、4倍に拡大して表示する / Display a 320x180 image enlarged 4 times.

    std::printf("keys: 1-6 experiment, W stripe width, H gray 188, C color pair, G gamma, B bands, R restart, S source, P print, Esc quit\n");

    int experiment = 1;
    Options options = {1, true, 0, true, false, 0};
    // true のフレームで、絵を描き直して、数字をコンソールに出す
    // Redraw the image on a true frame, then print the numbers to the console.
    bool changed = true;  
    bool restart = false;  // 実験4の積み上げを、最初からやり直すか / Whether to restart accumulation for experiment 4.
    bool was_down[256] = {};

    // 実験4で使う、色の合計と、足した回数。#08 と同じ
    // The color sum and sample count for experiment 4. Same as #08.
    std::vector<Vec3> sum(canvas.width * canvas.height, Vec3{0.0f, 0.0f, 0.0f});
    int sample_count = 0;

    while (canvas.update()) {
        if (canvas.is_key_down(key_escape)) {
            break;
        }

        for (int number = 1; number <= 6; number++) {
            if (key_pressed(canvas, '0' + number, was_down)) {
                experiment = number;
                changed = true;
                restart = (number == 4);
            }
        }

        // 切り替えのキーは、押したことを必ず覚えておく（その実験でなくても）。効くのは、その実験を見ているときだけ
        // Always remember key presses, even when their experiment is not active. Apply them only while viewing that experiment.
        const bool w_pressed = key_pressed(canvas, 'W', was_down);
        const bool h_pressed = key_pressed(canvas, 'H', was_down);
        const bool c_pressed = key_pressed(canvas, 'C', was_down);
        const bool g_pressed = key_pressed(canvas, 'G', was_down);
        const bool b_pressed = key_pressed(canvas, 'B', was_down);
        const bool r_pressed = key_pressed(canvas, 'R', was_down);
        const bool s_pressed = key_pressed(canvas, 'S', was_down);
        const bool p_pressed = key_pressed(canvas, 'P', was_down);

        if (experiment == 1 && w_pressed) {
            options.stripe_width = (options.stripe_width == 1) ? 2 : 1;
            changed = true;
        }
        if (experiment == 1 && h_pressed) {
            options.show_188 = !options.show_188;
            changed = true;
        }
        if (experiment == 3 && c_pressed) {
            options.color_pair = (options.color_pair + 1) % color_pair_count;
            changed = true;
        }
        if (experiment == 4 && g_pressed) {
            options.gamma = !options.gamma;
            changed = true;
        }
        if (experiment == 4 && b_pressed) {
            options.bands = !options.bands;
            changed = true;
        }
        if (experiment == 4 && r_pressed) {
            restart = true;
            changed = true;
        }
        if (experiment == 6 && s_pressed) {
            options.source_pattern = (options.source_pattern + 1) % 3;
            changed = true;
        }
        if (p_pressed) {
            changed = true;
        }

        if (experiment == 4) {
            // 積み上げをやり直すときは、乱数も最初に戻す。毎回、同じ順番で絵ができあがる
            // Reset the random number generator when restarting accumulation so each image uses the same sequence of samples.
            if (restart) {
                std::fill(sum.begin(), sum.end(), Vec3{0.0f, 0.0f, 0.0f});
                sample_count = 0;
                random_state = 1;
                restart = false;
            }
            sample_count++;
            accumulate_frame(canvas, sum, sample_count, options.gamma, options.bands);
            draw_marks(canvas, experiment, options);
            if (changed) {
                print_raytracer(canvas, options.gamma, options.bands, sample_count);
            }
        } else if (changed) {
            switch (experiment) {
            case 1:
                experiment_stripes(canvas, options.stripe_width, options.show_188);
                break;
            case 2:
                experiment_gradient(canvas);
                break;
            case 3:
                experiment_mix(canvas, color_pairs[options.color_pair]);
                break;
            case 5:
                experiment_steps(canvas);
                break;
            case 6:
                experiment_shrink(canvas, options.source_pattern);
                break;
            default:
                break;
            }
            draw_marks(canvas, experiment, options);
        }
        changed = false;

        canvas.present();
    }
}
