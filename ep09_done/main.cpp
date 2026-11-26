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

// 光の量（リニア。0〜1）を、画面に出す数字（sRGB。0〜1）に直す
// 画面は、数字に比例した光り方をしない。暗いところに数字を多めに割り振る決まりがある
// Convert a linear light value from 0 to 1 to an sRGB screen value from 0 to 1. A screen does not emit light in proportion to its values, so more values are assigned to dark shades.
float linear_to_srgb(float x) {
    // 0 のすぐ近くだけは、直線でつなぐ
    // Use a straight line only very close to 0.
    if (x <= 0.0031308f) {
        return x * 12.92f;
    }
    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;  // だいたい「1 / 2.2 乗」 / About "1 / 2.2 power".
}

// 0〜1 の色（Vec3 の x, y, z が赤, 緑, 青）を、画面に出す 0〜255 の Color に直す
// 計算は全部、光の量のまま進める。画面に出す直前のここで、1回だけ sRGB に直す
// Convert a color from 0 to 1 (Vec3 x, y, z are red, green, blue) to a screen Color from 0 to 255. Keep calculations in light values and convert to sRGB just once here, right before showing the image.
Color to_color(Vec3 c) {
    // 0〜1 の外の値をそのまま 255 倍して uint8_t にすると、値が化けてしまう。先に 0〜1 に切り詰める
    // Converting a value outside 0 to 1 directly to uint8_t after multiplying by 255 can wrap it. Clamp it to 0 to 1 first.
    const float r = linear_to_srgb(std::clamp(c.x, 0.0f, 1.0f));
    const float g = linear_to_srgb(std::clamp(c.y, 0.0f, 1.0f));
    const float b = linear_to_srgb(std::clamp(c.z, 0.0f, 1.0f));
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

// 光線の向きだけで決まる、空の色。この回から、空が光のもとになる
// The sky color depends only on the ray direction. Starting in this episode, the sky is the light source.
Vec3 sky_color(Ray ray) {
    const Vec3 white = {1.0f, 1.0f, 1.0f};
    const Vec3 sky_blue = {0.5f, 0.7f, 1.0f};

    const float t = 0.5f * (ray.direction.y + 1.0f);  // 下向き 0 〜 上向き 1 / Down is 0; up is 1.
    return mix(white, sky_blue, t);  // t が大きいほど青 / A larger t is more blue.
}

// 材料の種類。光がどう跳ね返るかが変わる
// Material type. It changes how light reflects.
enum class Kind {
    // 拡散。光をいろいろな向きに跳ね返す。紙や粘土のような、ざらざらの面
    // Diffuse reflection sends light in many directions. It is like a rough surface such as paper or clay.
    diffuse,  
    // 金属。鏡と同じ向きに跳ね返す。鏡は、ざらつきが 0 の金属として扱う
    // Metal reflects in the mirror direction. Treat a mirror as metal with fuzz 0.
    metal,  
};

// 材料。球ごとに1つ持つ
// A material. Each sphere has one.
struct Material {
    Kind kind;
    // 0〜1 の色。光を、赤・緑・青のそれぞれ何割返すか（アルベド）。金属では、跳ね返した光に付く色になる
    // A color from 0 to 1. It sets how much red, green, and blue light is returned (albedo). For metal, it colors the reflected light.
    Vec3 color;  
    // 金属のざらつき。0 でピカピカ、大きいほどくもる。拡散では使わない
    // Metal fuzz. At 0 it is shiny; larger values make it duller. Not used for diffuse materials.
    float fuzz = 0.0f;  
};

// 球。中心と半径と材料で表す
// Sphere. Defined by its center, radius, and material.
struct Sphere {
    Vec3 center;
    float radius;
    Material material;
    bool has_frame = false;  // true なら、鏡の壁として、輪の形の枠が付く / When true, add a ring-shaped frame to the wall mirror.
    // 枠の色。枠の上では、光を跳ね返さずに、この色をそのまま返す
    // The frame color. On the frame, return this color without reflecting light.
    Vec3 frame_color = {0.0f, 0.0f, 0.0f};  
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
// 使う数は sphere_count で決める
//   4 … 赤・鏡・緑の球と地面（前回までの場面）
//   6 … さらに、金とくもった銀の金属の球が加わる
//   8 … さらに、最後の2つ（巨大な鏡）が加わって合わせ鏡になる
// All spheres are listed here. sphere_count sets how many to use. 4: the earlier scene with red, mirror, and green spheres plus the ground. 6: add gold and rough silver metal spheres. 8: add the last two huge mirror walls to make a parallel mirror scene.
const int sphere_count = 6;
const Sphere spheres[] = {
    {Vec3{-1.9f, 0.0f, 3.8f}, 1.0f, Material{Kind::diffuse, Vec3{0.9f, 0.1f, 0.1f}}},
    // 真ん中の鏡の球。ざらつき 0 の金属。色は、光を 9 割返す灰色（鏡も、光を 100% は跳ね返さない）
    // The center mirror sphere. Smooth metal (fuzz 0), with a gray color that reflects 90% of the light. Real mirrors do not reflect 100% either.
    {Vec3{0.0f, 0.0f, 5.0f}, 1.0f, Material{Kind::metal, Vec3{0.9f, 0.9f, 0.9f}, 0.0f}},
    {Vec3{1.4f, 0.0f, 7.5f}, 1.0f, Material{Kind::diffuse, Vec3{0.1f, 0.8f, 0.2f}}},
    // 半径1000の球。大きすぎて、近くは平らに見える。上の端が y = -1 になる位置に置く
    // A sphere with radius 1000. It is so large that it looks flat nearby. Place it so its top edge is at y = -1.
    {Vec3{0.0f, -1001.0f, 0.0f}, 1000.0f, Material{Kind::diffuse, Vec3{0.8f, 0.8f, 0.75f}}},
    // ここから先が、この回で足した金属の球
    // 右の金の球はピカピカ（ざらつき 0）。金は、赤をよく返して、緑を少し、青をほとんど返さない
    // The metal spheres added in this episode start here. The gold ball on the right is shiny (fuzz 0). Gold reflects red well, a little green, and almost no blue.
    {Vec3{2.5f, 0.0f, 4.2f}, 1.0f, Material{Kind::metal, Vec3{1.0f, 0.65f, 0.2f}, 0.0f}},
    // 手前の小さな銀の球は、ざらつき 0.3 でくもる。半径が 0.5 なので、地面に乗せるには中心の y が -0.5
    // The small silver sphere in front is rough (fuzz 0.3). Its radius is 0.5, so put its center at y = -0.5 to rest on the ground.
    {Vec3{-0.9f, -0.5f, 2.9f}, 0.5f, Material{Kind::metal, Vec3{0.95f, 0.93f, 0.88f}, 0.3f}},
    // ここから先は、合わせ鏡の壁。カメラを内側に入れて、へこんだ鏡を前と後ろに向かい合わせる
    // 枠の色は、後ろの壁が水色、前の壁が金色
    // The parallel mirror walls start here. Put the camera inside, facing two concave mirrors opposite each other. The frame colors are cyan for the back wall and gold for the front wall.
    {Vec3{0.0f, 0.0f, 990.0f}, 1000.0f, Material{Kind::metal, Vec3{0.9f, 0.9f, 0.9f}, 0.0f}, true, Vec3{0.1f, 0.8f, 0.9f}},
    {Vec3{0.0f, 0.0f, -990.0f}, 1000.0f, Material{Kind::metal, Vec3{0.9f, 0.9f, 0.9f}, 0.0f}, true, Vec3{1.0f, 0.8f, 0.2f}},
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

// 壁の鏡に付ける、輪の形の枠。軸（カメラの正面を通る線）からの距離がこの範囲の場所は、鏡にならず枠になる
// 枠があると、鏡に映った鏡が何段目まで続いているかが、色で見える
// The ring-shaped frame on a mirror wall. Locations whose distance from the axis (the line through the camera and straight ahead) falls in this range become frame instead of mirror. The frame color shows how many mirror reflections deep the image goes.
const float frame_inner = 5.0f;
const float frame_outer = 8.0f;

// 跳ね返る回数の上限。鏡に映った鏡に映った……や、ざらざらの面で跳ね返り続ける光線が、いつまでも終わらないようにする
// The maximum number of bounces, so rays cannot continue forever between mirrors or bounce forever on rough surfaces.
const int max_depth = 20;

// 光線が当たった面で、どの向きに跳ね返るかを決める。跳ね返る光線を bounced に入れて true を返す
// 光が面の下にもぐって、吸収されたことにするときは、false を返す。その場合、bounced は触らない
// Choose the bounce direction for a ray that hits a surface. Store the bounced ray in bounced and return true. If the ray goes below the surface and is absorbed, return false without changing bounced.
bool bounce(Ray ray, Hit hit, Ray& bounced) {
    const Material material = spheres[hit.index].material;
    Vec3 direction = {0.0f, 0.0f, 0.0f};

    if (material.kind == Kind::metal) {
        // 金属は、鏡と同じ向きに跳ね返る。ざらつき（fuzz）の分だけ、向きをランダムにずらす
        // ずらす大きさは、ランダムな向き（長さ1）の fuzz 倍。fuzz が 0 なら、ずれずに鏡のままになる
        // Metal reflects in the mirror direction, then randomly offsets it by fuzz. The offset has the size of a random unit direction times fuzz. If fuzz is 0, there is no offset and it stays a mirror.
        direction = reflect(ray.direction, hit.normal) + random_direction() * material.fuzz;

        // ずらしたせいで、面の下にもぐってしまうことがある。そのときは、光が吸収されたことにして、光を返さない
        // 入ってくる向きと出ていく向きは、面をはさんで反対側のはず。法線との内積の符号が同じなら、面の下にもぐっている
        // 外から当たる球も、中から当たる壁も、これで調べられる
        // The offset can move the ray below the surface. Treat the light as absorbed and return no light. The incoming and outgoing directions should be on opposite sides of the surface; matching signs for their dot products with the normal mean the ray went below it. This works for spheres hit from outside and walls hit from inside.
        if (dot(ray.direction, hit.normal) * dot(direction, hit.normal) >= 0.0f) {
            return false;
        }
    } else {
        // ざらざらの面（拡散反射）は、光をいろいろな向きに跳ね返す。どこへ跳ねるか分からないので、向きはランダムに1つ選ぶ
        // Rough surfaces (diffuse reflection) send light in many directions. Since we do not know which way it goes, choose a random direction.
        direction = random_direction();

        // 面の内側を向いたら、ひっくり返す。内側に向かった光線は、面の中にもぐってしまう
        // If it points into the surface, flip it. A ray pointing inward would go below the surface.
        if (dot(direction, hit.normal) < 0.0f) {
            direction = -direction;
        }
    }

    // ずらしたあとは長さが 1 ではなくなるので、そろえる。光線の向きは、いつも長さ1にしておく
    // The offset changes the length from 1, so normalize it. Ray directions must always have length 1.
    bounced = Ray{hit.point, normalize(direction)};
    return true;
}

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

    // 鏡の壁でも、枠の上は跳ね返さない。枠の色を、そのまま返す
    // Even on a mirror wall, do not reflect on the frame. Return the frame color as is.
    if (spheres[hit.index].has_frame) {
        const float from_axis = length(Vec3{hit.point.x, hit.point.y, 0.0f});
        if (from_axis >= frame_inner && from_axis <= frame_outer) {
            return spheres[hit.index].frame_color;
        }
    }

    // どの向きに跳ね返るかは、材料で決まる。吸収されたら、光は届かない
    // 出発点から 0.001 より近い当たりは無視しているおかげで、跳ね返った光線は自分の表面に当たらない
    // The material decides which way the ray bounces. If it is absorbed, no light reaches us. Ignoring hits closer than 0.001 to the ray origin keeps a bounced ray from hitting its own surface.
    Ray bounced = {};
    if (!bounce(ray, hit, bounced)) {
        return Vec3{0.0f, 0.0f, 0.0f};
    }

    // 跳ね返った先の光を、もう一度 trace で調べる（関数が自分自身を呼ぶ。再帰）
    // 持ち帰った光に、材料の色を掛ける。赤い面なら赤い光を多めに、金なら黄色っぽい光を返す
    // Trace the bounced ray again (the function calls itself recursively). Multiply the returned light by the material color: a red surface returns more red light, and gold returns yellowish light.
    return spheres[hit.index].material.color * trace(bounced, depth + 1);
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

// 確かめ用の帯。画面の左上に、同じ大きさの帯を2本、横に並べる
//   左：白と黒を1ピクセルずつ交互に並べた縞。光の量は、ちょうど白の半分になる
//   右：光の量 0.5 のグレー
// 帯の外は、color をそのまま返す
// Test bands: place two equal-sized horizontal bands side by side in the top-left corner. Left: alternating white and black pixels, with half the light value of white. Right: gray with light value 0.5. Outside the bands, return color unchanged.
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

int main() {
    Canvas canvas(320, 180, 4);  // 320×180 の絵を、4倍に拡大して表示する / Display a 320x180 image enlarged 4 times.

    const float width = static_cast<float>(canvas.width);
    const float height = static_cast<float>(canvas.height);
    const float aspect = width / height;  // 画面の縦横比。320 ÷ 180 / The screen aspect ratio: 320 / 180.

    // true にすると、画面の左上に確かめ用の帯が出る。ガンマ補正の確かめに使う
    // When true, show test bands in the top-left corner of the screen. Use them to check gamma correction.
    const bool show_bands = false;

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
                sum[i] = sum[i] + trace(ray, 0);

                // 表示するのは、合計ではなく平均。合計を、足した回数で割る
                // Show the average, not the sum. Divide the sum by the sample count.
                Vec3 average = sum[i] * (1.0f / static_cast<float>(sample_count));
                if (show_bands) {
                    average = gray_bands(x, y, average);
                }
                set_pixel(canvas, x, y, to_color(average));
            }
        }

        canvas.present();
    }
}
