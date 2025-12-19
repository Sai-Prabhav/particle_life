#include <SFML/System.hpp>
#include <random>

inline int randInt(int n) {
  static std::mt19937 rng{std::random_device{}()};
  return static_cast<int>(rng() % n);
}

inline float randFloat01() {
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<float> dist(
      0.0f, 1.0f);
  return dist(rng);
}

inline float randFloat(float a, float b) {
  return a + (b - a) * randFloat01();
}

inline sf::Vector2f randPos(float h, float w, float r) {
  return sf::Vector2f{randFloat(r, w - r),
                      randFloat(r, h - r)};
}

inline std::vector<float> randFloats(float a, float b,
                                     int n) {
  std::vector<float> res(n);
  for (int i = 0; i < n; i++) {
    res[i] = randFloat(a, b);
  }
  return res;
}
