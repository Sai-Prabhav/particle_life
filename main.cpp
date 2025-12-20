#include "my_rand.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

class Particles : public sf::Drawable,
                  public sf::Transformable {

public:
  float drag;
  struct ParticleType {
    float mass;
    sf::Color color;
    float radius;
    std::vector<float> coff;
    std::vector<std::vector<float>> abc;
  };

  enum class Shape { Point, Triangle };
  enum class Edge { Pass, Bounce, Curve };

  Shape shape = Shape::Triangle;
  Edge edge = Edge::Bounce;
  float cutoff = 100.f;
  int numRegions = 3;
  std::vector<std::vector<float>> generateAbc() {
    std::vector<std::vector<float>> res(numRegions, {0, 0, 0});
    float c = 0.0f;
    for (int j = 0; j < numRegions; j++) {
      float next = randFloat(0, cutoff / numRegions);
    }
    return res;
  }
  static std::vector<ParticleType> randTypes(int typeCount) {
    std::vector<ParticleType> typeData(typeCount);
    for (int i = 0; i < typeCount; i++) {
      typeData[i] = randType(typeCount);
    }
    return typeData;
  }

  static ParticleType randType(int typeCount) {

    return {randFloat(0.1f, 1.0f),
            {static_cast<uint8_t>(randInt(205) + 50),
             static_cast<uint8_t>(randInt(205) + 50),
             static_cast<uint8_t>(randInt(205) + 50)},
            5,
            randFloats(-0.01f, 0.01f, typeCount)};
  }

  Particles(int particleCount, int typeCount, Shape shape_,
            Edge edge_, float w, float h, float drag_)
      : typeData(typeCount), typeId(particleCount),
        velocities(particleCount, {0.0f, 0.0f}),
        positions(particleCount) {
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

      sf::Vector2f p =
          randPos(h, w, typeData[typeId[i]].radius);
      positions[i] = p;
      velocities[i] = {randFloat(0.1f, 1.0f),
                       randFloat(0.1f, 1.0f)};
    }
  }

  void reset(float w, float h) {

    int particleCount = velocities.size();
    int typeCount = typeData.size();
    typeData = randTypes(typeCount);

    for (int i = 0; i < particleCount; i++) {
      typeId[i] = randInt(typeCount);

      sf::Vector2f p =
          randPos(h, w, typeData[typeId[i]].radius);
      positions[i] = p;
      velocities[i] = {randFloat(0.1f, 1.0f),
                       randFloat(0.1f, 1.0f)};
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
#pragma omp parallel for
    for (int i = 0; i < velocities.size(); i++) {
      sf::Vector2f force{0, 0};

      for (int j = 0; j < velocities.size(); j++) {
        if (i == j) {
          continue;
        }
        float cutoff2 = cutoff * cutoff;

        sf::Vector2f diff = positions[i] - positions[j];
        float d2 = diff.lengthSquared();

        if (d2 > cutoff2 || d2 < 1e-6f)
          continue;

        force += (typeData[typeId[i]].coff[typeId[j]] *
                  typeData[typeId[j]].mass * diff) *
                 calculateForce(d2);
      }
      // force += -vel[i] * drag;
      velocities[i] += force * elapsed.asSeconds();

      sf::Vector2f dis = velocities[i] * elapsed.asSeconds();
      moveVertex(i, dis);
      handleBoundary(i, width, height);
      velocities[i] *=
          std::exp(std::log(drag) * elapsed.asSeconds());
    }
  }

private:
  sf::VertexArray m_vertices;
  std::vector<ParticleType> typeData;
  std::vector<int> typeId;
  std::vector<sf::Vector2f> velocities;
  std::vector<sf::Vector2f> positions;

  inline float calculateForce(float r) {

    return 1 / (r * r) * (r - 1) * (r - 50);
    return 500000 * std::exp(r * -0.000001) *
           std::sin(sf::priv::pi * 2 * numRegions / cutoff *
                    r) /
           r;
  }

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
      if (positions[i].x < 0) {
        velocities[i].x = std::abs(velocities[i].x);
      }
      if (positions[i].y < 0) {

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
      std::cout << "Color: " << static_cast<int>(p.color.r)
                << " " << static_cast<int>(p.color.g) << " "
                << static_cast<int>(p.color.b) << "\n";
    }

    for (ParticleType p : typeData) {
      std::cout << "Coff: ";
      for (auto coffi : p.coff) {
        if (coffi >= 0.0f) {
          std::cout << " ";
        }
        std::cout << std::fixed << std::setprecision(3) << coffi
                  << "  ";
      }
      std::cout << "\n";
    }
  }

  void draw(sf::RenderTarget &target,
            sf::RenderStates states) const override {

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

void handleInput(sf::RenderWindow *window,
                 Particles *particles) {

  window->handleEvents(
      [&window](const sf::Event::Closed &) { window->close(); },
      [&](const sf::Event::MouseButtonPressed &mouse) {
        std::cout << "Mouse clicked at: " << mouse.position.x
                  << ", " << mouse.position.y << "\n";
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

void draw(sf::RenderWindow *window, Particles *par,
          sf::Time time, sf::Text *fpsText) {
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

  sf::RenderWindow window(sf::VideoMode({300, 300}),
                          "My window");
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
                      Particles::Shape::Triangle,
                      Particles::Edge::Curve, w, h, drag);
  sf::Time totalTime(sf::Time::Zero);
  sf::Clock clock;
  int frameNumber = 0;

  window.draw(infoText);
  while (window.isOpen()) {

    auto time = clock.restart();
    totalTime += time;
    frameNumber += 1;

    if (totalTime.asSeconds() > 0.1f) {
      int fps =
          static_cast<int>(frameNumber / totalTime.asSeconds());
      infoText.setString(
          "FPS: " + std::to_string(fps) +
          "\nSpeed: " + std::to_string(speed) + "\nDrag: " +
          std::to_string(particles.drag) + "\nRange: " +
          std::to_string(static_cast<int>(particles.cutoff)));
      totalTime = sf::Time::Zero;
      frameNumber = 0;
    }
    // std::cout << 1 / time.asSeconds() << "\n";
    handleInput(&window, &particles);
    particles.update(time * speed, w, h);
    draw(&window, &particles, time, &infoText);
  }
}
