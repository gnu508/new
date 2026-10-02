#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadiusX = 1.0f;
constexpr float kRadiusY = 0.91f;
constexpr float kRadiusZ = 0.88f;
constexpr float kFruitHeight = 1.02f;

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 v, float s) { return {v.x * s, v.y * s, v.z * s}; }
Vec3 operator/(Vec3 v, float s) { return {v.x / s, v.y / s, v.z / s}; }

float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float length(Vec3 v) { return std::sqrt(dot(v, v)); }
Vec3 normalize(Vec3 v) {
    const float magnitude = length(v);
    return magnitude > 0.00001f ? v / magnitude : Vec3{0.0f, 1.0f, 0.0f};
}

struct Mat4 {
    float value[16]{};
};

Mat4 identity() {
    Mat4 result{};
    result.value[0] = result.value[5] = result.value[10] = result.value[15] = 1.0f;
    return result;
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            for (int index = 0; index < 4; ++index) {
                result.value[column * 4 + row] +=
                    a.value[index * 4 + row] * b.value[column * 4 + index];
            }
        }
    }
    return result;
}

Mat4 translation(Vec3 position) {
    Mat4 result = identity();
    result.value[12] = position.x;
    result.value[13] = position.y;
    result.value[14] = position.z;
    return result;
}

Mat4 scaling(Vec3 amount) {
    Mat4 result = identity();
    result.value[0] = amount.x;
    result.value[5] = amount.y;
    result.value[10] = amount.z;
    return result;
}

Mat4 rotationX(float angle) {
    Mat4 result = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    result.value[5] = c;
    result.value[6] = s;
    result.value[9] = -s;
    result.value[10] = c;
    return result;
}

Mat4 rotationY(float angle) {
    Mat4 result = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    result.value[0] = c;
    result.value[2] = -s;
    result.value[8] = s;
    result.value[10] = c;
    return result;
}

Mat4 rotationZ(float angle) {
    Mat4 result = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    result.value[0] = c;
    result.value[1] = s;
    result.value[4] = -s;
    result.value[5] = c;
    return result;
}

Mat4 perspective(float fov, float aspect, float nearPlane, float farPlane) {
    Mat4 result{};
    const float scale = 1.0f / std::tan(fov * 0.5f);
    result.value[0] = scale / aspect;
    result.value[5] = scale;
    result.value[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.value[11] = -1.0f;
    result.value[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return result;
}

Mat4 lookAt(Vec3 eye, Vec3 target) {
    const Vec3 forward = normalize(target - eye);
    const Vec3 side = normalize(cross(forward, {0.0f, 1.0f, 0.0f}));
    const Vec3 up = cross(side, forward);
    Mat4 result = identity();
    result.value[0] = side.x;
    result.value[1] = up.x;
    result.value[2] = -forward.x;
    result.value[4] = side.y;
    result.value[5] = up.y;
    result.value[6] = -forward.y;
    result.value[8] = side.z;
    result.value[9] = up.z;
    result.value[10] = -forward.z;
    result.value[12] = -dot(side, eye);
    result.value[13] = -dot(up, eye);
    result.value[14] = dot(forward, eye);
    return result;
}

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec3 color;
};

struct Mesh {
    GLuint vao{};
    GLuint vbo{};
    GLsizei count{};
    GLenum primitive{GL_TRIANGLES};

    void upload(const std::vector<Vertex>& vertices, GLenum drawMode = GL_TRIANGLES) {
        primitive = drawMode;
        count = static_cast<GLsizei>(vertices.size());
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                     vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, color)));
        glBindVertexArray(0);
    }
};

struct Body {
    Mesh mesh;
    Vec3 origin;
    Vec3 position;
    Vec3 velocity{};
    Vec3 rotation{};
    Vec3 spin{};
};

struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec3 color;
    float life{};
    float size{};
};

Vec3 fruitPoint(float latitude, float longitude, float radial = 1.0f) {
    const float ring = std::cos(latitude) * radial;
    return {kRadiusX * ring * std::sin(longitude), kRadiusY * std::sin(latitude) * radial,
            kRadiusZ * ring * std::cos(longitude)};
}

void triangle(std::vector<Vertex>& vertices, Vec3 a, Vec3 b, Vec3 c, Vec3 color,
              Vec3 normalHint) {
    Vec3 normal = normalize(cross(b - a, c - a));
    if (dot(normal, normalHint) < 0.0f) {
        std::swap(b, c);
        normal = normal * -1.0f;
    }
    vertices.push_back({a, normal, color});
    vertices.push_back({b, normal, color});
    vertices.push_back({c, normal, color});
}

void quad(std::vector<Vertex>& vertices, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 color,
          Vec3 normalHint) {
    triangle(vertices, a, b, c, color, normalHint);
    triangle(vertices, a, c, d, color, normalHint);
}

Vec3 fleshColor(float radial) {
    if (radial > 0.96f) return {0.20f, 0.49f, 0.22f};
    if (radial > 0.89f) return {0.76f, 0.83f, 0.43f};
    const float variation = 0.92f + 0.08f * std::sin(radial * 31.0f);
    return {0.88f * variation, 0.12f * variation, 0.16f * variation};
}

void addSeed(std::vector<Vertex>& vertices, Vec3 center, float tilt) {
    constexpr int rings = 5;
    constexpr int sides = 7;
    const Vec3 seedColor{0.10f, 0.035f, 0.025f};
    for (int ring = 0; ring < rings; ++ring) {
        const float a0 = -kPi * 0.5f + kPi * static_cast<float>(ring) / rings;
        const float a1 = -kPi * 0.5f + kPi * static_cast<float>(ring + 1) / rings;
        for (int side = 0; side < sides; ++side) {
            const float p0 = 2.0f * kPi * static_cast<float>(side) / sides;
            const float p1 = 2.0f * kPi * static_cast<float>(side + 1) / sides;
            const auto point = [&](float latitude, float longitude) {
                const float x = 0.021f * std::cos(latitude) * std::cos(longitude);
                const float y = 0.047f * std::sin(latitude);
                const float z = 0.014f * std::cos(latitude) * std::sin(longitude);
                return center + Vec3{x * std::cos(tilt) - y * std::sin(tilt),
                                     x * std::sin(tilt) + y * std::cos(tilt), z};
            };
            const Vec3 a = point(a0, p0), b = point(a1, p0);
            const Vec3 c = point(a1, p1), d = point(a0, p1);
            const Vec3 hint = normalize(((a + b + c + d) / 4.0f) - center);
            quad(vertices, a, b, c, d, seedColor, hint);
        }
    }
}

std::vector<Vertex> buildWedge(float latitude0, float latitude1, float longitude0,
                               float longitude1, Vec3 center) {
    std::vector<Vertex> vertices;
    constexpr int surfaceLatSteps = 5;
    constexpr int surfaceLongSteps = 4;
    constexpr int cutRadialSteps = 6;
    constexpr int cutLatSteps = 5;
    constexpr int capRadialSteps = 5;
    constexpr int capLongSteps = 5;

    const Vec3 stripeDark{0.045f, 0.28f, 0.12f};
    const Vec3 stripeLight{0.11f, 0.52f, 0.20f};

    for (int latStep = 0; latStep < surfaceLatSteps; ++latStep) {
        const float lat0 = latitude0 + (latitude1 - latitude0) * latStep / surfaceLatSteps;
        const float lat1 = latitude0 + (latitude1 - latitude0) * (latStep + 1) / surfaceLatSteps;
        for (int longStep = 0; longStep < surfaceLongSteps; ++longStep) {
            const float lon0 = longitude0 + (longitude1 - longitude0) * longStep / surfaceLongSteps;
            const float lon1 = longitude0 + (longitude1 - longitude0) * (longStep + 1) / surfaceLongSteps;
            const float stripe = std::sin((lon0 + lon1) * 4.5f + 0.2f);
            const Vec3 color = stripe > 0.42f ? stripeDark : stripeLight;
            const Vec3 a = fruitPoint(lat0, lon0) - center;
            const Vec3 b = fruitPoint(lat1, lon0) - center;
            const Vec3 c = fruitPoint(lat1, lon1) - center;
            const Vec3 d = fruitPoint(lat0, lon1) - center;
            const Vec3 middle = (a + b + c + d) / 4.0f;
            const Vec3 normal{middle.x / (kRadiusX * kRadiusX),
                              (middle.y + center.y) / (kRadiusY * kRadiusY),
                              middle.z / (kRadiusZ * kRadiusZ)};
            quad(vertices, a, b, c, d, color, normal);
        }
    }

    for (int side = 0; side < 2; ++side) {
        const float longitude = side == 0 ? longitude0 : longitude1;
        const Vec3 normalHint{(side == 0 ? -1.0f : 1.0f) * std::cos(longitude), 0.0f,
                              (side == 0 ? 1.0f : -1.0f) * std::sin(longitude)};
        for (int radialStep = 0; radialStep < cutRadialSteps; ++radialStep) {
            const float radial0 = static_cast<float>(radialStep) / cutRadialSteps;
            const float radial1 = static_cast<float>(radialStep + 1) / cutRadialSteps;
            for (int latStep = 0; latStep < cutLatSteps; ++latStep) {
                const float lat0 = latitude0 + (latitude1 - latitude0) * latStep / cutLatSteps;
                const float lat1 = latitude0 + (latitude1 - latitude0) * (latStep + 1) / cutLatSteps;
                const Vec3 a = fruitPoint(lat0, longitude, radial0) - center;
                const Vec3 b = fruitPoint(lat1, longitude, radial0) - center;
                const Vec3 c = fruitPoint(lat1, longitude, radial1) - center;
                const Vec3 d = fruitPoint(lat0, longitude, radial1) - center;
                const float radial = (radial0 + radial1) * 0.5f;
                quad(vertices, a, b, c, d, fleshColor(radial), normalHint);
            }
        }

        if (side == 1) {
            for (int seed = 0; seed < 2; ++seed) {
                const float latitude = latitude0 + (latitude1 - latitude0) * (0.35f + 0.3f * seed);
                const Vec3 seedCenter = fruitPoint(latitude, longitude, 0.53f) - center;
                addSeed(vertices, seedCenter, latitude * 1.7f);
            }
        }
    }

    for (int end = 0; end < 2; ++end) {
        const float latitude = end == 0 ? latitude0 : latitude1;
        const Vec3 normalHint{0.0f, end == 0 ? -1.0f : 1.0f, 0.0f};
        for (int radialStep = 0; radialStep < capRadialSteps; ++radialStep) {
            const float radial0 = static_cast<float>(radialStep) / capRadialSteps;
            const float radial1 = static_cast<float>(radialStep + 1) / capRadialSteps;
            for (int longStep = 0; longStep < capLongSteps; ++longStep) {
                const float lon0 = longitude0 + (longitude1 - longitude0) * longStep / capLongSteps;
                const float lon1 = longitude0 + (longitude1 - longitude0) * (longStep + 1) / capLongSteps;
                const Vec3 a = fruitPoint(latitude, lon0, radial0) - center;
                const Vec3 b = fruitPoint(latitude, lon1, radial0) - center;
                const Vec3 c = fruitPoint(latitude, lon1, radial1) - center;
                const Vec3 d = fruitPoint(latitude, lon0, radial1) - center;
                quad(vertices, a, b, c, d, fleshColor((radial0 + radial1) * 0.5f), normalHint);
            }
        }
    }

    return vertices;
}

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        std::string log(static_cast<size_t>(size), '\0');
        glGetShaderInfoLog(shader, size, nullptr, log.data());
        std::cerr << "Shader compilation failed: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createProgram() {
    constexpr const char* vertexSource = R"GLSL(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;
        uniform mat4 uMVP;
        uniform mat4 uModel;
        out vec3 vPosition;
        out vec3 vNormal;
        out vec3 vColor;
        void main() {
            vec4 worldPosition = uModel * vec4(aPosition, 1.0);
            vPosition = worldPosition.xyz;
            vNormal = mat3(uModel) * aNormal;
            vColor = aColor;
            gl_Position = uMVP * vec4(aPosition, 1.0);
        }
    )GLSL";
    constexpr const char* fragmentSource = R"GLSL(
        #version 330 core
        in vec3 vPosition;
        in vec3 vNormal;
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            vec3 normal = normalize(vNormal);
            vec3 lightDirection = normalize(vec3(-0.45, 0.82, 0.56));
            float diffuse = max(dot(normal, lightDirection), 0.0);
            vec3 viewDirection = normalize(vec3(0.0, 2.4, 7.0) - vPosition);
            vec3 halfDirection = normalize(lightDirection + viewDirection);
            float specular = pow(max(dot(normal, halfDirection), 0.0), 36.0) * 0.24;
            float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0) * 0.13;
            vec3 litColor = vColor * (0.30 + diffuse * 0.82) + vec3(specular + rim);
            FragColor = vec4(litColor, 1.0);
        }
    )GLSL";

    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) return 0;
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        std::string log(static_cast<size_t>(size), '\0');
        glGetProgramInfoLog(program, size, nullptr, log.data());
        std::cerr << "Shader link failed: " << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

Mat4 bodyMatrix(Vec3 position, Vec3 rotation) {
    const Mat4 rotationMatrix = multiply(rotationZ(rotation.z),
                                         multiply(rotationY(rotation.y), rotationX(rotation.x)));
    return multiply(translation(position), rotationMatrix);
}

class Demo {
public:
    explicit Demo(GLFWwindow* window) : window_(window), random_(std::random_device{}()) {
        program_ = createProgram();
        mvpLocation_ = glGetUniformLocation(program_, "uMVP");
        modelLocation_ = glGetUniformLocation(program_, "uModel");
        buildFruit();
        buildEnvironment();
        buildParticleMesh();
        updateTitle();
    }

    void render(int width, int height) {
        if (height <= 0 || width <= 0) return;
        glViewport(0, 0, width, height);
        glClearColor(0.025f, 0.055f, 0.052f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Vec3 target{0.0f, 0.92f, 0.0f};
        const float horizontal = cameraDistance_ * std::cos(cameraPitch_);
        const Vec3 eye = target + Vec3{std::sin(cameraYaw_) * horizontal,
                                      std::sin(cameraPitch_) * cameraDistance_,
                                      std::cos(cameraYaw_) * horizontal};
        const Mat4 viewProjection = multiply(
            perspective(0.76f, static_cast<float>(width) / height, 0.08f, 60.0f),
            lookAt(eye, target));

        glUseProgram(program_);
        draw(floor_, viewProjection, identity());
        draw(grid_, viewProjection, identity());
        for (const Body& body : bodies_) {
            draw(body.mesh, viewProjection, bodyMatrix(body.position, body.rotation));
        }
        for (const Particle& particle : particles_) {
            const Mat4 model = multiply(translation(particle.position),
                                        scaling({particle.size, particle.size, particle.size}));
            draw(particleMesh_, viewProjection, model);
        }
    }

    void update(float deltaTime) {
        deltaTime = std::min(deltaTime, 0.035f);
        if (exploded_) {
            for (Body& body : bodies_) {
                body.velocity.y -= 9.8f * deltaTime;
                body.position = body.position + body.velocity * deltaTime;
                body.rotation = body.rotation + body.spin * deltaTime;
                if (body.position.y < 0.16f) {
                    body.position.y = 0.16f;
                    if (body.velocity.y < -0.45f) body.velocity.y *= -0.34f;
                    else body.velocity.y = 0.0f;
                    body.velocity.x *= 0.86f;
                    body.velocity.z *= 0.86f;
                    body.spin = body.spin * 0.985f;
                }
            }
            for (Particle& particle : particles_) {
                particle.life -= deltaTime;
                particle.velocity.y -= 9.8f * deltaTime;
                particle.position = particle.position + particle.velocity * deltaTime;
            }
            particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                             [](const Particle& p) { return p.life <= 0.0f; }),
                              particles_.end());
        }
    }

    void explode(Vec3 impactDirection) {
        if (exploded_) return;
        exploded_ = true;
        impactDirection = normalize(impactDirection);
        std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
        for (Body& body : bodies_) {
            const Vec3 radial = normalize(body.origin - Vec3{0.0f, kFruitHeight, 0.0f});
            const Vec3 direction = normalize(radial * 0.75f + impactDirection * 0.58f +
                                             Vec3{0.0f, 0.3f, 0.0f});
            body.velocity = direction * strength_ +
                            Vec3{jitter(random_) * strength_ * 0.18f,
                                 strength_ * (0.33f + 0.28f * (jitter(random_) + 1.0f)),
                                 jitter(random_) * strength_ * 0.18f};
            body.spin = {jitter(random_) * 7.0f, jitter(random_) * 7.0f,
                         jitter(random_) * 7.0f};
        }
        for (int index = 0; index < 115; ++index) {
            const float longitude = jitter(random_) * kPi;
            const float height = jitter(random_) * 1.6f;
            const Vec3 direction = normalize({std::sin(longitude), height, std::cos(longitude)});
            const Vec3 color = index % 3 == 0 ? Vec3{0.90f, 0.15f, 0.17f}
                                              : Vec3{0.27f, 0.66f, 0.22f};
            particles_.push_back({Vec3{0.0f, kFruitHeight, 0.0f} + direction * 0.48f,
                                  direction * (strength_ * (0.5f + std::abs(jitter(random_)))) +
                                      impactDirection * strength_ * 0.4f,
                                  color, 1.0f + std::abs(jitter(random_)) * 1.2f,
                                  0.018f + std::abs(jitter(random_)) * 0.026f});
        }
        updateTitle();
    }

    void reset() {
        exploded_ = false;
        particles_.clear();
        for (Body& body : bodies_) {
            body.position = body.origin;
            body.velocity = {};
            body.rotation = {};
            body.spin = {};
        }
        updateTitle();
    }

    void changeStrength(float amount) {
        strength_ = std::clamp(strength_ + amount, 2.0f, 9.0f);
        updateTitle();
    }

    void orbit(float deltaX, float deltaY) {
        cameraYaw_ += deltaX * 0.006f;
        cameraPitch_ = std::clamp(cameraPitch_ + deltaY * 0.005f, -0.2f, 1.1f);
    }

    void zoom(float amount) {
        cameraDistance_ = std::clamp(cameraDistance_ - amount * 0.32f, 3.0f, 12.0f);
    }

private:
    void draw(const Mesh& mesh, const Mat4& viewProjection, const Mat4& model) const {
        const Mat4 mvp = multiply(viewProjection, model);
        glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, mvp.value);
        glUniformMatrix4fv(modelLocation_, 1, GL_FALSE, model.value);
        glBindVertexArray(mesh.vao);
        glDrawArrays(mesh.primitive, 0, mesh.count);
    }

    void buildFruit() {
        constexpr int longitudeCount = 14;
        constexpr int latitudeBands = 5;
        for (int band = 0; band < latitudeBands; ++band) {
            const float latitude0 = -kPi * 0.5f + kPi * band / latitudeBands;
            const float latitude1 = -kPi * 0.5f + kPi * (band + 1) / latitudeBands;
            for (int wedge = 0; wedge < longitudeCount; ++wedge) {
                const float longitude0 = 2.0f * kPi * wedge / longitudeCount;
                const float longitude1 = 2.0f * kPi * (wedge + 1) / longitudeCount;
                const Vec3 origin = fruitPoint((latitude0 + latitude1) * 0.5f,
                                               (longitude0 + longitude1) * 0.5f, 0.42f) +
                                    Vec3{0.0f, kFruitHeight, 0.0f};
                Body body{};
                body.origin = origin;
                body.position = origin;
                const auto vertices = buildWedge(latitude0, latitude1, longitude0,
                                                 longitude1, origin - Vec3{0.0f, kFruitHeight, 0.0f});
                body.mesh.upload(vertices);
                bodies_.push_back(std::move(body));
            }
        }
    }

    void buildEnvironment() {
        std::vector<Vertex> floorVertices;
        const Vec3 floorColor{0.045f, 0.095f, 0.084f};
        quad(floorVertices, {-12.0f, 0.0f, -12.0f}, {-12.0f, 0.0f, 12.0f},
             {12.0f, 0.0f, 12.0f}, {12.0f, 0.0f, -12.0f}, floorColor, {0.0f, 1.0f, 0.0f});
        floor_.upload(floorVertices);

        std::vector<Vertex> gridVertices;
        const Vec3 gridColor{0.085f, 0.16f, 0.14f};
        for (int index = -12; index <= 12; ++index) {
            const float coordinate = static_cast<float>(index) * 0.5f;
            const Vec3 majorColor = index % 4 == 0 ? Vec3{0.13f, 0.23f, 0.19f} : gridColor;
            gridVertices.push_back({{-6.0f, 0.006f, coordinate}, {0.0f, 1.0f, 0.0f}, majorColor});
            gridVertices.push_back({{6.0f, 0.006f, coordinate}, {0.0f, 1.0f, 0.0f}, majorColor});
            gridVertices.push_back({{coordinate, 0.006f, -6.0f}, {0.0f, 1.0f, 0.0f}, majorColor});
            gridVertices.push_back({{coordinate, 0.006f, 6.0f}, {0.0f, 1.0f, 0.0f}, majorColor});
        }
        grid_.upload(gridVertices, GL_LINES);
    }

    void buildParticleMesh() {
        std::vector<Vertex> vertices;
        const Vec3 color{0.8f, 0.3f, 0.15f};
        const Vec3 top{0.0f, 1.0f, 0.0f}, right{1.0f, 0.0f, 0.0f};
        const Vec3 front{0.0f, 0.0f, 1.0f}, left{-1.0f, 0.0f, 0.0f};
        const Vec3 back{0.0f, 0.0f, -1.0f}, bottom{0.0f, -1.0f, 0.0f};
        triangle(vertices, top, front, right, color, {1.0f, 1.0f, 1.0f});
        triangle(vertices, top, right, back, color, {1.0f, 1.0f, -1.0f});
        triangle(vertices, top, back, left, color, {-1.0f, 1.0f, -1.0f});
        triangle(vertices, top, left, front, color, {-1.0f, 1.0f, 1.0f});
        triangle(vertices, bottom, right, front, color, {1.0f, -1.0f, 1.0f});
        triangle(vertices, bottom, back, right, color, {1.0f, -1.0f, -1.0f});
        triangle(vertices, bottom, left, back, color, {-1.0f, -1.0f, -1.0f});
        triangle(vertices, bottom, front, left, color, {-1.0f, -1.0f, 1.0f});
        particleMesh_.upload(vertices);
    }

    void updateTitle() const {
        const std::string state = exploded_ ? "FRAGMENTED" : "INTACT";
        const std::string title = "WATERMELON // IMPACT LAB   [" + state + "]   |   SPACE: hit   R: reset   UP/DOWN: force " +
                                  std::to_string(static_cast<int>(strength_));
        glfwSetWindowTitle(window_, title.c_str());
    }

    GLFWwindow* window_{};
    GLuint program_{};
    GLint mvpLocation_{-1};
    GLint modelLocation_{-1};
    std::vector<Body> bodies_;
    std::vector<Particle> particles_;
    Mesh floor_;
    Mesh grid_;
    Mesh particleMesh_;
    std::mt19937 random_;
    bool exploded_{};
    float strength_{5.0f};
    float cameraYaw_{0.0f};
    float cameraPitch_{0.27f};
    float cameraDistance_{6.3f};
};

struct InputState {
    Demo* demo{};
    bool dragging{};
    bool moved{};
    double lastX{};
    double lastY{};
    double pressX{};
    double pressY{};
};

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_SPACE) input->demo->explode({0.0f, 0.25f, 1.0f});
    if (key == GLFW_KEY_R) input->demo->reset();
    if (key == GLFW_KEY_UP || key == GLFW_KEY_EQUAL) input->demo->changeStrength(1.0f);
    if (key == GLFW_KEY_DOWN || key == GLFW_KEY_MINUS) input->demo->changeStrength(-1.0f);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        input->dragging = true;
        input->moved = false;
        glfwGetCursorPos(window, &input->lastX, &input->lastY);
        input->pressX = input->lastX;
        input->pressY = input->lastY;
    } else if (action == GLFW_RELEASE && input->dragging) {
        input->dragging = false;
        if (!input->moved) {
            int width = 1, height = 1;
            glfwGetWindowSize(window, &width, &height);
            const double normalizedX = input->pressX / width * 2.0 - 1.0;
            input->demo->explode({static_cast<float>(normalizedX) * 0.7f, 0.25f, 1.0f});
        }
    }
}

void cursorCallback(GLFWwindow* window, double x, double y) {
    auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
    if (!input->dragging) return;
    const double dx = x - input->lastX;
    const double dy = y - input->lastY;
    if (std::abs(x - input->pressX) + std::abs(y - input->pressY) > 5.0) input->moved = true;
    input->demo->orbit(static_cast<float>(dx), static_cast<float>(dy));
    input->lastX = x;
    input->lastY = y;
}

void scrollCallback(GLFWwindow* window, double, double y) {
    auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
    input->demo->zoom(static_cast<float>(y));
}

}  // namespace

static int runDemo() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Could not initialize GLFW. Check your window-system dependencies.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 820, "Watermelon Impact Lab", nullptr, nullptr);
    if (!window) {
        std::cerr << "Could not create an OpenGL 3.3 window. Check your display and drivers.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
        std::cerr << "Could not load OpenGL functions.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);

    Demo demo(window);
    InputState input{&demo};
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetScrollCallback(window, scrollCallback);

    double previousTime = glfwGetTime();
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double currentTime = glfwGetTime();
        demo.update(static_cast<float>(currentTime - previousTime));
        previousTime = currentTime;
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        demo.render(width, height);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int main() {
    return runDemo();
}

#ifdef _WIN32
extern "C" int __stdcall WinMain(void*, void*, char*, int) {
    return runDemo();
}
#endif