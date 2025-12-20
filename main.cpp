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
      : typeData(typeCount), type(particleCount),
        vel(particleCount, {0.0f, 0.0f}), pos(particleCount) {
    if (shape == Shape::Point) {
      m_vertices.resize(vel.size());
      m_vertices.setPrimitiveType(sf::PrimitiveType::Points);
    } else if (shape == Shape::Triangle) {

      m_vertices.resize(vel.size() * 3);

      m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    }
    edge = edge_;
    shape = shape_;
    drag = drag_;
    typeData = randTypes(typeCount);
    initVertices();
    printTypes();

    for (int i = 0; i < particleCount; i++) {
      type[i] = randInt(typeCount);

      sf::Vector2f p = randPos(h, w, typeData[type[i]].radius);
      pos[i] = p;
      vel[i] = {randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f)};
    }
  }

  void reset(float w, float h) {

    int particleCount = vel.size();
    int typeCount = typeData.size();
    typeData = randTypes(typeCount);

    for (int i = 0; i < particleCount; i++) {
      type[i] = randInt(typeCount);

      sf::Vector2f p = randPos(h, w, typeData[type[i]].radius);
      pos[i] = p;
      vel[i] = {randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f)};
    }
    printTypes();
    initVertices();
  }

  void printPos() {
    for (int i = 0; i < vel.size(); i++) {
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
    for (int i = 0; i < vel.size(); i++) {
      sf::Vector2f force{0, 0};

      for (int j = 0; j < vel.size(); j++) {
        if (i == j) {
          continue;
        }
        float cutoff2 = cutoff * cutoff;

        sf::Vector2f diff = pos[i] - pos[j];
        float d2 = diff.lengthSquared();

        if (d2 > cutoff2 || d2 < 1e-6f)
          continue;

        force += (typeData[type[i]].coff[type[j]] *
                  typeData[type[j]].mass * diff) *
                 func(d2);
      }
      // force += -vel[i] * drag;
      vel[i] += force * elapsed.asSeconds();

      sf::Vector2f dis = vel[i] * elapsed.asSeconds();
      moveVertice(i, dis);
      handelBoundry(i, width, height);
      vel[i] *= std::exp(std::log(drag) * elapsed.asSeconds());
    }
  }

private:
  sf::VertexArray m_vertices;
  std::vector<ParticleType> typeData;
  std::vector<int> type;
  std::vector<sf::Vector2f> vel;
  std::vector<sf::Vector2f> pos;

  inline float func(float r) {

    return 1 / (r * r) * (r - 1) * (r - 50);
    return 500000 * std::exp(r * -0.000001) *
           std::sin(sf::priv::pi * 2 * numRegions / cutoff *
                    r) /
           r;
  }

  void initVertices() {

    for (int i = 0; i < vel.size(); i++) {

      if (shape == Shape::Point) {
        m_vertices[i].position = pos[i];
        m_vertices[i].color = typeData[type[i]].color;
      } else if (shape == Shape::Triangle) {
        auto r = typeData[type[i]].radius;
        float th = std::sqrt(3.f) * r / 2.f;

        m_vertices[i * 3].position = {-r / 2, -th / 3};
        m_vertices[i * 3 + 1].position = {r / 2, -th / 3};
        m_vertices[i * 3 + 2].position = {0, 2 * th / 3};

        m_vertices[i * 3].position += pos[i];
        m_vertices[i * 3 + 1].position += pos[i];
        m_vertices[i * 3 + 2].position += pos[i];

        m_vertices[i * 3].color = typeData[type[i]].color;
        m_vertices[i * 3 + 1].color = typeData[type[i]].color;
        m_vertices[i * 3 + 2].color = typeData[type[i]].color;
      }
    }
  }

  inline void moveVertice(int i, sf::Vector2f dis) {
    pos[i] += dis;
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
    for (int i = 0; i < vel.size(); i++) {
      sf::Vector2f dis = vel[i] * elapsed.asSeconds();
      moveVertice(i, dis);
    }
  }

  inline void handelBoundry(int i, float width, float height) {

    if (edge == Edge::Bounce) {
      if (pos[i].x < 0) {
        vel[i].x = std::abs(vel[i].x);
      }
      if (pos[i].y < 0) {

        vel[i].y = std::abs(vel[i].y);
      }
      if (pos[i].x > (width - 10)) {

        vel[i].x = -std::abs(vel[i].x);
      }
      if (pos[i].y > (height - 10)) {

        vel[i].y = -std::abs(vel[i].y);
      }
    } else if (edge == Edge::Pass) {
      return;
    } else if (edge == Edge::Curve) {
      if (pos[i].x < 0) {
        moveVertice(i, {width, 0});
      } else if (pos[i].x > (width)) {
        moveVertice(i, {-width, 0});
      }
      if (pos[i].y < 0) {
        moveVertice(i, {0, height});
      } else if (pos[i].y > height) {

        moveVertice(i, {0, -height});
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

int numPar;
int numTypes;
float drag = 0.05, speed = 1;
float w, h;
bool isClean = true;

void handelInput(sf::RenderWindow *window, Particles *par) {

  window->handleEvents(
      [&window](const sf::Event::Closed &) { window->close(); },
      [&](const sf::Event::MouseButtonPressed &mouse) {
        std::cout << "Mouse clicked at: " << mouse.position.x
                  << ", " << mouse.position.y << "\n";
        par->reset(w, h);
      },
      [&](const sf::Event::KeyPressed &key) {
        if (key.code == sf::Keyboard::Key::Escape) {
          window->close();
        }

        if (key.code == sf::Keyboard::Key::R) {
          std::cout << "R pressed -> reset\n";
          par->reset(w, h);

          window->clear(sf::Color::Black);
        }

        if (key.code == sf::Keyboard::Key::RBracket) {
          speed += 0.25;
        }
        if (key.code == sf::Keyboard::Key::LBracket) {
          speed -= 0.25;
        }
        if (key.code == sf::Keyboard::Key::Semicolon) {
          par->drag *= 0.8;
        }
        if (key.code == sf::Keyboard::Key::Apostrophe) {
          par->drag *= 1.18;
        }
        if (key.code == sf::Keyboard::Key::Period) {
          par->cutoff += 10.0f;
        }
        if (key.code == sf::Keyboard::Key::Comma) {
          par->cutoff -= 10.0f;
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

void draw(sf::RenderWindow *win, Particles *par, sf::Time time,
          sf::Text *fpsText) {
  if (isClean) {
    win->clear(sf::Color::Black);
  }
  win->draw(*par);
  win->draw(*fpsText);
  win->display();
}

int main(int argc, char **argv) {

  sf::Font font;
  if (!font.openFromFile("font.ttf")) {
    std::cerr << "Failed to load font\n";
    return 1;
  }

  sf::Text fpsText(font);
  fpsText.setCharacterSize(16);
  fpsText.setFillColor(sf::Color::White);
  fpsText.setPosition({10.f, 10.f});
  fpsText.setString("102");

  sf::RenderWindow window(sf::VideoMode({300, 300}),
                          "My window");
  if (argc != 2) {
    std::cout << "Usage main numberParticle numberTypes";
    return -1;
  }
  numPar = std::stoi(argv[1]);
  numTypes = std::stoi(argv[2]);

  sf::Vector2u size = window.getSize();
  w = static_cast<float>(size.x);
  h = static_cast<float>(size.y);
  Particles par(numPar, numTypes, Particles::Shape::Triangle,
                Particles::Edge::Curve, w, h, drag);
  sf::Time totelTime(sf::Time::Zero);
  sf::Clock clock;
  int frameNumber = 0;

  window.draw(fpsText);
  while (window.isOpen()) {

    auto time = clock.restart();
    totelTime += time;
    frameNumber += 1;

    if (totelTime.asSeconds() > 0.1f) {
      int fps =
          static_cast<int>(frameNumber / totelTime.asSeconds());
      fpsText.setString(
          "FPS: " + std::to_string(fps) +
          "\nSpeed: " + std::to_string(speed) +
          "\nDrag: " + std::to_string(par.drag) + "\nRange: " +
          std::to_string(static_cast<int>(par.cutoff)));
      totelTime = sf::Time::Zero;
      frameNumber = 0;
    }
    // std::cout << 1 / time.asSeconds() << "\n";
    handelInput(&window, &par);
    par.update(time * speed, w, h);
    draw(&window, &par, time, &fpsText);
  }
}
