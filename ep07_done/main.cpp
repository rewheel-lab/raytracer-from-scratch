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

// 3つの成分を、まとめて s 倍する
// Multiply all three components by s.
Vec3 operator*(Vec3 v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
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

// 0〜1 の色（Vec3 の x, y, z が赤, 緑, 青）を、画面に出す 0〜255 の Color に直す
// Convert a color from 0 to 1 (Vec3 x, y, z are red, green, blue) to a screen Color from 0 to 255.
Color to_color(Vec3 c) {
    // 0〜1 の外の値をそのまま 255 倍して uint8_t にすると、値が化けてしまう。先に 0〜1 に切り詰める
    // Converting a value outside 0 to 1 directly to uint8_t after multiplying by 255 can wrap it. Clamp it to 0 to 1 first.
    const float r = std::clamp(c.x, 0.0f, 1.0f);
    const float g = std::clamp(c.y, 0.0f, 1.0f);
    const float b = std::clamp(c.z, 0.0f, 1.0f);
    return Color{
        static_cast<uint8_t>(r * 255.0f + 0.5f),  // + 0.5 で四捨五入する / Add 0.5 to round to the nearest integer.
        static_cast<uint8_t>(g * 255.0f + 0.5f),
        static_cast<uint8_t>(b * 255.0f + 0.5f)};
}

// a と b を混ぜる。t が 0 なら a、1 なら b、その間は割合で混ぜる
// Mix a and b. When t is 0, use a; when t is 1, use b; otherwise, mix them by that amount.
Vec3 mix(Vec3 a, Vec3 b, float t) {
    return a * (1.0f - t) + b * t;
}

// 光線の向きだけで決まる、空の色。太陽は光が来る向き（to_light）に置く。光が動いたら、太陽も一緒に動く
// The sky color depends only on the ray direction. Put the sun in the direction of the incoming light (to_light), so it moves with the light.
Vec3 sky_color(Ray ray, Vec3 to_light) {
    const Vec3 white = {1.0f, 1.0f, 1.0f};
    const Vec3 sky_blue = {0.1f, 0.3f, 0.95f};
    const Vec3 sun_color = {1.0f, 0.95f, 0.6f};

    // 向きがほぼ太陽と同じ光線だけ、太陽の色にする。0.998 に近いほど小さな太陽になる
    // Only rays almost parallel to the sun direction use the sun color. The closer this value is to 0.998, the smaller the sun appears.
    if (dot(ray.direction, to_light) > 0.998f) {
        return sun_color;
    }

    const float t = 0.5f * (ray.direction.y + 1.0f);  // 下向き 0 〜 上向き 1 / Down is 0; up is 1.
    return mix(white, sky_blue, t);  // t が大きいほど青 / A larger t is more blue.
}

// 球。中心と半径と色で表す
// Sphere. Defined by its center, radius, and color.
struct Sphere {
    Vec3 center;
    float radius;
    // 0〜1 の色。鏡の球では使わない。壁の鏡では、枠の色になる
    // A color from 0 to 1. Not used by mirror spheres; for wall mirrors, it is the frame color.
    Vec3 color;  
    bool mirror = false;  // true なら、光を跳ね返す鏡になる / When true, this becomes a mirror that reflects light.
    bool casts_shadow = true;  // false なら、この球は影を作らない / If false, this sphere does not cast a shadow.
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

// 球は全部ここに並べる
// 使う数は sphere_count で決める。6 にすると、最後の2つ（巨大な鏡）が加わって合わせ鏡になる
// All spheres are listed here. Setting sphere_count to 6 adds the last two huge mirror walls to make a parallel mirror scene.
const int sphere_count = 4;
const Sphere spheres[] = {
    {Vec3{-1.9f, 0.0f, 3.8f}, 1.0f, Vec3{0.9f, 0.2f, 0.2f}},
    // 真ん中の鏡の球。鏡なので、色は使わない
    // The center mirror sphere. It is a mirror, so no color is used.
    {Vec3{0.0f, 0.0f, 5.0f}, 1.0f, Vec3{0.2f, 0.8f, 0.3f}, true},  
    {Vec3{1.4f, 0.0f, 7.5f}, 1.0f, Vec3{0.2f, 0.4f, 0.9f}},
    // 半径1000の球。大きすぎて、近くは平らに見える。上の端が y = -1 になる位置に置く
    // A sphere with radius 1000. It is so large that it looks flat nearby. Place it so its top edge is at y = -1.
    {Vec3{0.0f, -1001.0f, 0.0f}, 1000.0f, Vec3{0.8f, 0.8f, 0.75f}},
    // ここから先は、合わせ鏡の壁。カメラを内側に入れて、へこんだ鏡を前と後ろに向かい合わせる
    // 壁が光をさえぎると、中が真っ暗になる。だから影は作らない
    // 壁の color は、鏡のまわりの枠の色。後ろの壁が水色、前の壁が金色
    // The parallel mirror walls start here. Put the camera inside, facing two concave mirrors opposite each other. The walls block the light, so their inside would be black; do not cast shadows. The wall color is the frame color: cyan for the back wall and gold for the front wall.
    {Vec3{0.0f, 0.0f, 990.0f}, 1000.0f, Vec3{0.1f, 0.8f, 0.9f}, true, false},
    {Vec3{0.0f, 0.0f, -990.0f}, 1000.0f, Vec3{1.0f, 0.8f, 0.2f}, true, false},
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

// 当たった点の色。光が当たっている明るさと、影で決める
// The color at the hit point, based on how much light reaches it and whether it is in shadow.
Vec3 shade(Hit hit, Vec3 to_light) {
    // ランバートの式。明るさは、法線と光へ向かう向きの内積
    // 光と反対を向いた面は内積がマイナスになるが、光の量はマイナスにならないので 0 にする
    // Lambert's formula. Brightness is the dot product of the normal and the direction to the light. A surface facing away from the light has a negative dot product; clamp it to 0 because light cannot be negative.
    float brightness = std::max(0.0f, dot(hit.normal, to_light));

    // 影。当たった点から光へ向かう光線（シャドウレイ）を飛ばして、途中で何かに当たったら、光は届かない
    // t の最小値を 0.001 にして、出発した点のすぐ近くにある自分自身の表面を無視する
    // To check for shadows, send a ray (shadow ray) from the hit point toward the light. If it hits anything on the way, the light is blocked. Set t_min to 0.001 to ignore the surface at the ray origin.
    const Ray shadow_ray = {hit.point, to_light};
    Hit blocker = {};
    if (hit_scene(shadow_ray, 0.001f, blocker) && spheres[blocker.index].casts_shadow) {
        brightness = 0.0f;
    }

    return spheres[hit.index].color * brightness;
}

// 鏡に当たるたびに、明るさが何倍になるか。本物の鏡も、光を100% は跳ね返さない
// The brightness multiplier for each mirror bounce. A real mirror does not reflect 100% of the light either.
const float mirror_reflectance = 0.9f;

// 壁の鏡に付ける、輪の形の枠。軸（カメラの正面を通る線）からの距離がこの範囲の場所は、鏡にならず枠になる
// 枠があると、鏡に映った鏡が何段目まで続いているかが、色で見える
// The ring-shaped frame on a mirror wall. Locations whose distance from the axis (the line through the camera and straight ahead) falls in this range become frame instead of mirror. The frame color shows how many mirror reflections deep the image goes.
const float frame_inner = 5.0f;
const float frame_outer = 8.0f;

// 跳ね返る回数の上限。鏡に映った鏡に映った……が、いつまでも終わらないようにする
// The maximum number of bounces, so a mirror reflecting a mirror forever cannot keep the program running.
const int max_depth = 20;

// 光線が運んでくる色を求める。depth は、ここまでに何回跳ね返ったか
// Find the color carried by a ray. depth is the number of bounces so far.
Vec3 trace(Ray ray, Vec3 to_light, int depth) {
    // 自分を呼ぶ関数は、まず「どこで止まるか」を書く。上限まで跳ね返ったら、そこで打ち切る
    // For a recursive function, first define where it stops. Stop after reaching the bounce limit.
    if (depth >= max_depth) {
        return Vec3{0.0f, 0.0f, 0.0f};
    }

    // 当たらなければ空の色
    // If there is no hit, use the sky color.
    Hit hit = {};
    if (!hit_scene(ray, 0.001f, hit)) {
        return sky_color(ray, to_light);
    }

    if (spheres[hit.index].mirror) {
        // 鏡でも、枠の上は跳ね返さない。枠の色を、そのまま返す
        // 真ん中の鏡の球は、軸から 1 以内にしかないので、枠にはならない
        // Even for a mirror, do not reflect on the frame. Return the frame color as is. The center mirror sphere stays within 1 unit of the axis, so it is not part of the frame.
        const float from_axis = length(Vec3{hit.point.x, hit.point.y, 0.0f});
        if (from_axis >= frame_inner && from_axis <= frame_outer) {
            return spheres[hit.index].color;
        }

        // 鏡なら、跳ね返った光線で、もう一度 trace を呼ぶ（関数が自分自身を呼ぶ。再帰）
        // 出発点から 0.001 より近い当たりは無視しているおかげで、跳ね返った光線は自分の表面に当たらない
        // For a mirror, trace the reflected ray again (the function calls itself recursively). Ignoring hits closer than 0.001 to the ray origin keeps the bounced ray from hitting its own surface.
        const Ray reflected = {hit.point, reflect(ray.direction, hit.normal)};
        return trace(reflected, to_light, depth + 1) * mirror_reflectance;
    }

    return shade(hit, to_light);
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

int main() {
    Canvas canvas(320, 180, 4);  // 320×180 の絵を、4倍に拡大して表示する / Display a 320x180 image enlarged 4 times.

    const float width = static_cast<float>(canvas.width);
    const float height = static_cast<float>(canvas.height);
    const float aspect = width / height;  // 画面の縦横比。320 ÷ 180 / The screen aspect ratio: 320 / 180.

    // 光へ向かう向き。平行光源なので、どこでも同じ向き
    // この回は、結果を積み上げていく。絵が動くと前の結果と混ざってしまうので、光は動かさない
    // The direction toward the light. It is the same everywhere for a directional light. Keep it still while accumulating samples, or movement in the image would mix with earlier results.
    const Vec3 to_light = normalize(Vec3{std::sin(2.0f), 0.5f, std::cos(2.0f)});

    // 色の合計。ピクセルごとに1つ持ち、毎フレーム、新しい1回分を足していく
    // The color sum. Keep one per pixel and add one new sample each frame.
    std::vector<Vec3> sum(canvas.width * canvas.height, Vec3{0.0f, 0.0f, 0.0f});

    // 何回分を足したか。全部のピクセルで同じ回数なので、1つだけ持てばよい
    // How many samples have been added. All pixels use the same count, so one value is enough.
    int sample_count = 0;

    while (canvas.update()) {
        sample_count++;

        for (int y = 0; y < canvas.height; y++) {
            for (int x = 0; x < canvas.width; x++) {
                // ピクセルの中のランダムな位置を通る光線を作る。カメラは原点で、+Z を向く
                // 毎フレーム位置が変わるので、ピクセルの中のいろいろな場所を調べることになる
                // Make a ray through a random position inside the pixel. The camera is at the origin and faces +Z. Its position changes every frame, so we sample different places inside the pixel.
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
                sum[i] = sum[i] + trace(ray, to_light, 0);

                // 表示するのは、合計ではなく平均。合計を、足した回数で割る
                // Show the average, not the sum. Divide the sum by the sample count.
                const Vec3 average = sum[i] * (1.0f / static_cast<float>(sample_count));
                set_pixel(canvas, x, y, to_color(average));
            }
        }

        canvas.present();
    }
}
