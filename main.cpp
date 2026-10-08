#include "my_rand.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr double C = 100.0;
constexpr float sign(float x) { return x < 0 ? -1 : 1; }
constexpr float min(float x, float y) { return x < y ? x : y; }

class Particles : public sf::Drawable, public sf::Transformable {
public:
  float drag;
  struct ParticleType {
    float mass;
    sf::Color color;
    float radius;
    std::vector<float> coff;
    std::vector<float> coff_near;
    std::vector<float> coff_far;
  };

  enum class Shape { Point, Triangle };
  enum class Edge { Pass, Bounce, Curve };

  Shape shape = Shape::Triangle;
  Edge edge;
  float cutoff = 100;
  static std::vector<ParticleType> randTypes(int typeCount) {
    std::vector<ParticleType> typeData(typeCount);
    for (int i = 0; i < typeCount; i++) {
      typeData[i] = randType(typeCount);
    }
    return typeData;
  }

  static ParticleType randType(int typeCount) {

    return ParticleType{randFloat(0.1f, 1.0f),
                        {static_cast<uint8_t>(randInt(205) + 50),
                         static_cast<uint8_t>(randInt(205) + 50),
                         static_cast<uint8_t>(randInt(205) + 50)},
                        5,
                        randFloats(-1.f, 1.f, typeCount),
                        randFloats(100.f, 10.f, typeCount),
                        randFloats(-10.f, -1.f, typeCount)};
  }

  Particles(int particleCount, int typeCount, Shape shape_, Edge edge_, float w,
            float h, float drag_)
      : typeData(typeCount), typeId(particleCount),
        velocities(particleCount, {0.0f, 0.0f}), positions(particleCount) {
    if (shape == Shape::Point) {
      m_vertices.resize(velocities.size());
      m_vertices.setPrimitiveType(sf::PrimitiveType::Points);
    } else if (shape == Shape::Triangle) {

      m_vertices.resize(velocities.size() * 3);

      m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    }
    edge = edge_;
    shape = shape_;
    drag = drag_;
    typeData = randTypes(typeCount);
    initVertices();
    printTypes();

    for (int i = 0; i < particleCount; i++) {
      typeId[i] = randInt(typeCount);

      sf::Vector2f p = randPos(h, w, typeData[typeId[i]].radius);
      positions[i] = p;
      velocities[i] = {randFloat(-1.0f, 1.0f), randFloat(-1.0f, 1.0f)};
    }
  }

  void reset(float w, float h) {

    int particleCount = velocities.size();
    int typeCount = typeData.size();
    typeData = randTypes(typeCount);

    for (int i = 0; i < particleCount; i++) {
      typeId[i] = randInt(typeCount);

      sf::Vector2f p = randPos(h, w, typeData[typeId[i]].radius);
      positions[i] = p;
      velocities[i] = {randFloat(-1.0f, 1.0f), randFloat(1.0f, 1.0f)};
    }
    printTypes();
    initVertices();
  }

  void printPos() {
    for (int i = 0; i < velocities.size(); i++) {
      std::cout << i << " :";
      std::cout << m_vertices[i * 3].position.x << " , "
                << m_vertices[i * 3].position.y << " , ";
      std::cout << m_vertices[i * 3 + 1].position.x << " , "
                << m_vertices[i * 3 + 1].position.y << " , ";
      std::cout << m_vertices[i * 3 + 2].position.x << " , "
                << m_vertices[i * 3 + 2].position.y << "\n";
    }
  }

  void update(sf::Time elapsed, float width, float height) {
    double error = false;
    const float cutoff2 = cutoff * cutoff;
    const float cutoff_far2 = cutoff2 * 0.5f * 0.5f;
    const float cutoff_near2 = 2;
#pragma omp parallel for simd
    for (int i = 0; i < velocities.size(); i++) {
      float force_x = 0.0, force_y = 0.0;
      for (int j = 0; j < velocities.size(); j++) {

        float dx = positions[i].x - positions[j].x;
        float dy = positions[i].y - positions[j].y;
        float d2;
        if (edge == Edge::Curve) {
          if (std::abs(dx) > width / 2.0) {
            dx = -sign(dx) * (width - std::abs(dx));
          }
          if (std::abs(dy) > height / 2.0) {
            dy = -sign(dy) * (height - std::abs(dy));
          }
        }
        d2 = dx * dx + dy * dy;
        float cf = 0.0;
        if (d2 > cutoff2 || d2 < 1 || i == j) {
          continue;
        } else if (d2 > cutoff_far2) {
          cf = typeData[typeId[i]].coff_far[typeId[j]];
        } else if (d2 > cutoff_near2 && d2 <= cutoff_far2) {
          cf = typeData[typeId[i]].coff[typeId[j]];
        } else if (d2 <= cutoff_near2) {
          cf = typeData[typeId[i]].coff_near[typeId[j]];
        }
        float d = std::sqrt(d2 + 0.01);
        force_x +=
            (cf * typeData[typeId[j]].mass * dx) * calculateForce(d2) / d;
        force_y +=
            (cf * typeData[typeId[j]].mass * dy) * calculateForce(d2) / d;
      }

      /**
       * Calculate relativistic velocity with initial velocity v0
       *
       * Analytical solution to the relativistic equation of motion:
       *   dv/dt = F / (m * gamma^3)
       *
       * where gamma = 1 / sqrt(1 - v^2/c^2) is the Lorentz factor
       *
       * General solution with initial condition v(0) = v0:
       *   v(t) = (gamma_0 * m * v0 + F*t) / sqrt(m^2 + (gamma_0*m*v0
       * +F*t)^2/c^2)
       **/

      // std::cout << "total force done\n";
      sf::Vector2f force{force_x, force_y};
      auto v0 = velocities[i];
      auto f = force - v0 * drag;
      float m = typeData[typeId[i]].mass;
      float t = elapsed.asSeconds();

      float g0 = 1.0 / std::sqrt(1.0 - v0.lengthSquared() / (C * C));
      const sf::Vector2f vel1 = g0 * v0 + f * (t / m);
      velocities[i] = (vel1 / static_cast<float>(std::sqrt(
                                  1.0f + vel1.lengthSquared() / (C * C))));
      if (velocities[i].lengthSquared() >= C * C) {
        auto v1 = velocities[i];
        std::cout << "---------------------------------------------------------"
                     "----\n\n\n";
        std::cout << v1.length() / C << " " << (v0 * drag).length() << " "
                  << f.length() << "\n";
      }
    }
    for (int i = 0; i < velocities.size(); i++) {
      sf::Vector2f dis = velocities[i] * elapsed.asSeconds();
      moveVertex(i, dis);

      handleBoundary(i, width, height);
    }
  }

private:
  sf::VertexArray m_vertices;
  std::vector<ParticleType> typeData;
  std::vector<int> typeId;
  std::vector<sf::Vector2f> velocities;
  std::vector<sf::Vector2f> positions;

  inline float calculateForce(float r) { return 100000.0 / (0.1 + r); }

  void initVertices() {

    for (int i = 0; i < velocities.size(); i++) {

      if (shape == Shape::Point) {
        m_vertices[i].position = positions[i];
        m_vertices[i].color = typeData[typeId[i]].color;
      } else if (shape == Shape::Triangle) {
        auto r = typeData[typeId[i]].radius;
        float th = std::sqrt(3.f) * r / 2.f;

        m_vertices[i * 3].position = {-r / 2, -th / 3};
        m_vertices[i * 3 + 1].position = {r / 2, -th / 3};
        m_vertices[i * 3 + 2].position = {0, 2 * th / 3};

        m_vertices[i * 3].position += positions[i];
        m_vertices[i * 3 + 1].position += positions[i];
        m_vertices[i * 3 + 2].position += positions[i];

        m_vertices[i * 3].color = typeData[typeId[i]].color;
        m_vertices[i * 3 + 1].color = typeData[typeId[i]].color;
        m_vertices[i * 3 + 2].color = typeData[typeId[i]].color;
      }
    }
  }

  inline void moveVertex(int i, sf::Vector2f dis) {
    if (dis.lengthSquared() < -1) {
      std::cout << i << " " << dis.x << " " << dis.y << "vert "

                << m_vertices[i * 3].position.x << " "
                << m_vertices[i * 3].position.y << "\n";
    }
    positions[i] += dis;
    if (shape == Shape::Triangle) {

      m_vertices[i * 3].position += dis;
      m_vertices[i * 3 + 1].position += dis;
      m_vertices[i * 3 + 2].position += dis;
    } else if (shape == Shape::Point) {
      m_vertices[i].position += dis;
    }
  }
  void moveVertices(sf::Time elapsed) {

#pragma omp parallel for
    for (int i = 0; i < velocities.size(); i++) {
      sf::Vector2f dis = velocities[i] * elapsed.asSeconds();
      moveVertex(i, dis);
    }
  }

  inline void handleBoundary(int i, float width, float height) {

    if (edge == Edge::Bounce) {
      if (positions[i].x < 10) {
        velocities[i].x = std::abs(velocities[i].x);
      }
      if (positions[i].y < 10) {

        velocities[i].y = std::abs(velocities[i].y);
      }
      if (positions[i].x > (width - 10)) {

        velocities[i].x = -std::abs(velocities[i].x);
      }
      if (positions[i].y > (height - 10)) {

        velocities[i].y = -std::abs(velocities[i].y);
      }
    } else if (edge == Edge::Pass) {
      return;
    } else if (edge == Edge::Curve) {
      if (positions[i].x < 0) {
        moveVertex(i, {width, 0});
      } else if (positions[i].x > (width)) {
        moveVertex(i, {-width, 0});
      }
      if (positions[i].y < 0) {
        moveVertex(i, {0, height});
      } else if (positions[i].y > height) {

        moveVertex(i, {0, -height});
      }
    }
  }

  void printTypes() {
    std::cout << "Size: " << typeData.size() << "\n";
    for (ParticleType p : typeData) {
      std::cout << "Color: " << static_cast<int>(p.color.r) << " "
                << static_cast<int>(p.color.g) << " "
                << static_cast<int>(p.color.b) << "\n";
    }

    for (ParticleType p : typeData) {
      std::cout << "Coff: ";
      for (auto coffi : p.coff) {
        if (coffi >= 0.0f) {
          std::cout << " ";
        }
        std::cout << std::fixed << std::setprecision(3) << coffi << "  ";
      }
      std::cout << "\n";
    }
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override {

    states.transform *= getTransform();

    states.texture = nullptr;

    target.draw(m_vertices, states);
  }
};

int numParticles;
int numParticleTypes;
float drag = 0.05, speed = 1;
float w, h;
bool isClean = true;

void handleInput(sf::RenderWindow *window, Particles *particles) {

  window->handleEvents(
      [&window](const sf::Event::Closed &) { window->close(); },
      [&](const sf::Event::MouseButtonPressed &mouse) {
        std::cout << "Mouse clicked at: " << mouse.position.x << ", "
                  << mouse.position.y << "\n";
        particles->reset(w, h);
      },
      [&](const sf::Event::KeyPressed &key) {
        if (key.code == sf::Keyboard::Key::Escape) {
          window->close();
        }

        if (key.code == sf::Keyboard::Key::R) {
          std::cout << "R pressed -> reset\n";
          particles->reset(w, h);

          window->clear(sf::Color::Black);
        }

        if (key.code == sf::Keyboard::Key::RBracket) {
          speed += 0.25;
        }
        if (key.code == sf::Keyboard::Key::LBracket) {
          speed -= 0.25;
        }
        if (key.code == sf::Keyboard::Key::Semicolon) {
          particles->drag *= 0.8;
        }
        if (key.code == sf::Keyboard::Key::Apostrophe) {
          particles->drag *= 1.18;
        }
        if (key.code == sf::Keyboard::Key::Period) {
          particles->cutoff += 10.0f;
        }
        if (key.code == sf::Keyboard::Key::Comma) {
          particles->cutoff -= 10.0f;
        }
        if (key.code == sf::Keyboard::Key::C) {
          isClean = !isClean;
        }
      },
      [&](const sf::Event::Resized &e) {
        w = static_cast<float>(e.size.x);
        h = static_cast<float>(e.size.y);

        sf::View view(sf::FloatRect({0, 0}, {w, h}));
        window->setView(view);
        std::cout << "Resized: " << w << " " << h << "\n";
      });
}

void draw(sf::RenderWindow *window, Particles *par, sf::Time time,
          sf::Text *fpsText) {
  if (isClean) {
    window->clear(sf::Color::Black);
  }
  window->draw(*par);
  window->draw(*fpsText);
  window->display();
}

int main(int argc, char **argv) {

  sf::Font font;
  if (!font.openFromFile("font.ttf")) {
    std::cerr << "Failed to load font\n";
    return 1;
  }

  sf::Text infoText(font);
  infoText.setCharacterSize(16);
  infoText.setFillColor(sf::Color::White);
  infoText.setPosition({10.f, 10.f});
  infoText.setString("102");

  sf::RenderWindow window(sf::VideoMode({300, 300}), "My window");
  if (argc != 3) {
    std::cout << "Usage main numberParticle numberTypes";
    return -1;
  }
  numParticles = std::stoi(argv[1]);
  numParticleTypes = std::stoi(argv[2]);

  sf::Vector2u size = window.getSize();
  w = static_cast<float>(size.x);
  h = static_cast<float>(size.y);
  Particles particles(numParticles, numParticleTypes,
                      Particles::Shape::Triangle, Particles::Edge::Curve, w, h,
                      drag);
  sf::Time totalTime(sf::Time::Zero);
  sf::Clock clock;
  int frameNumber = 0;

  window.draw(infoText);
  while (window.isOpen()) {

    auto time = clock.restart();
    totalTime += time;
    frameNumber += 1;

    if (totalTime.asSeconds() > 0.1f) {
      int fps = static_cast<int>(frameNumber / totalTime.asSeconds());
      infoText.setString(
          "FPS: " + std::to_string(fps) + "\nSpeed: " + std::to_string(speed) +
          "\nDrag: " + std::to_string(particles.drag) +
          "\nRange: " + std::to_string(static_cast<int>(particles.cutoff)));
      totalTime = sf::Time::Zero;
      frameNumber = 0;
    }

    // std::cout << 1 / time.asSeconds() << "\n";
    handleInput(&window, &particles);

    particles.update(time * speed, w, h);
    draw(&window, &particles, time, &infoText);
  }
}
