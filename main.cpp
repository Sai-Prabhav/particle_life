#include "my_rand.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <vector>

class Particles : public sf::Drawable, public sf::Transformable {
public:
  struct ParticleType {
    float mass;
    float drag;
    sf::Color color;
    float radius;
    std::vector<float> coff;
  };

  enum class Shape { Point, Triangle };

  static std::vector<ParticleType> randTypes(int typeCount, float drag) {
    std::vector<ParticleType> typeData(typeCount);
    for (int i = 0; i < typeCount; i++) {

      typeData[i] = randType(typeCount, drag);
    }
    return typeData;
  }

  static ParticleType randType(int typeCount, float drag) {

    return {randFloat(0.1f, 1.0f),
            drag,
            {static_cast<uint8_t>(randInt(205) + 50),
             static_cast<uint8_t>(randInt(205) + 50),
             static_cast<uint8_t>(randInt(205) + 50)},
            5,
            randFloats(-0.01f, 0.01f, typeCount)};
  }

  Particles(int particleCount, int typeCount, Shape shape_, float w, float h,
            float drag)
      : typeData(typeCount), type(particleCount), vel(particleCount, {0.0f, 0.0f}),
        pos(particleCount) {
    if (shape == Shape::Point) {
      m_vertices.resize(vel.size());
      m_vertices.setPrimitiveType(sf::PrimitiveType::Points);
    } else if (shape == Shape::Triangle) {

      m_vertices.resize(vel.size() * 3);

      m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    }
    shape = shape_;
    typeData = randTypes(typeCount, drag);
    initVertices();
    printTypes();
    for (int i = 0; i < particleCount; i++) {
      type[i] = randInt(typeCount);

      sf::Vector2f p = randPos(h, w, typeData[type[i]].radius);
      pos[i] = p;
      vel[i] = {randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f)};
    }
  }
  void reset(float w, float h, float drag) {

    int particleCount = vel.size();
    int typeCount = typeData.size();
    typeData = randTypes(typeCount, drag);

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
        float cutoff = 400.f;
        float cutoff2 = cutoff * cutoff;

        sf::Vector2f diff = pos[i] - pos[j];
        float d2 = diff.lengthSquared();

        if (d2 > cutoff2 || d2 < 1e-6f)
          continue;

        float x = d2;
        force += (typeData[type[i]].coff[type[j]] * typeData[type[i]].mass * 100 *
                  typeData[type[j]].mass * diff) *
                 (1 / (x * x) * (x - 50) * (x - 1));
      }

      vel[i] += force * elapsed.asSeconds() / typeData[type[i]].mass;
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
      vel[i] *= std::exp(std::log(typeData[type[i]].drag) * elapsed.asSeconds());
    }
    moveVertices(elapsed);
  }

private:
  sf::VertexArray m_vertices;
  std::vector<ParticleType> typeData;
  std::vector<int> type;
  std::vector<sf::Vector2f> vel;
  std::vector<sf::Vector2f> pos;
  Shape shape = Shape::Point;

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
  void moveVertices(sf::Time elapsed) {

#pragma omp parallel for
    for (int i = 0; i < vel.size(); i++) {
      sf::Vector2f dis = vel[i] * elapsed.asSeconds();
      pos[i] += dis;
      if (shape == Shape::Triangle) {

        m_vertices[i * 3].position += dis;
        m_vertices[i * 3 + 1].position += dis;
        m_vertices[i * 3 + 2].position += dis;
      } else if (shape == Shape::Point) {
        m_vertices[i].position += dis;
      }
    }
  }

  void printTypes() {
    std::cout << "Size: " << typeData.size() << "\n";
    for (ParticleType p : typeData) {
      std::cout << "Color: " << static_cast<int>(p.color.r) << " "
                << static_cast<int>(p.color.g) << " " << static_cast<int>(p.color.b)
                << "\n";
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

int numPar;
int numTypes;
float drag, speed;
float w, h;

void handelInput(sf::RenderWindow *window, Particles *par) {
  window->handleEvents([&window](const sf::Event::Closed &) { window->close(); },
                       [&](const sf::Event::MouseButtonPressed &mouse) {
                         std::cout << "Mouse clicked at: " << mouse.position.x
                                   << ", " << mouse.position.y << "\n";
                         par->reset(w, h, drag);
                       },
                       [&](const sf::Event::Resized &e) {
                         w = static_cast<float>(e.size.x);
                         h = static_cast<float>(e.size.y);

                         sf::View view(sf::FloatRect({0, 0}, {w, h}));
                         window->setView(view);
                         std::cout << w << " " << h << "\n";
                       });
}

void draw(sf::RenderWindow *win, Particles *par, sf::Time time, sf::Text *fpsText) {
  win->clear(sf::Color::Black);
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

  sf::RenderWindow window(sf::VideoMode({300, 300}), "My window");
  numPar = std::stoi(argv[1]);
  numTypes = std::stoi(argv[2]);
  drag = std::stof(argv[3]);
  speed = std::stof(argv[4]);

  sf::Vector2u size = window.getSize();
  w = static_cast<float>(size.x);
  h = static_cast<float>(size.y);
  Particles par(numPar, numTypes, Particles::Shape::Point, w, h, drag);
  sf::Time totelTime(sf::Time::Zero);
  sf::Clock clock;
  int frameNumber = 0;

  window.draw(fpsText);
  while (window.isOpen()) {

    auto time = clock.restart();
    totelTime += time;
    frameNumber += 1;

    if (totelTime.asSeconds() > 0.1f) {
      int fps = static_cast<int>(frameNumber / totelTime.asSeconds());
      fpsText.setString(std::to_string(fps));
      totelTime = sf::Time::Zero;
      frameNumber = 0;
    }
    // std::cout << 1 / time.asSeconds() << "\n";
    handelInput(&window, &par);
    par.update(time * speed, w, h);
    draw(&window, &par, time, &fpsText);
  }
}
