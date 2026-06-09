#include <nomlib/core.hpp>
#include <nomlib/math.hpp>
#include <nomlib/platforms.hpp>
#include <nomlib/version.hpp>
#include <iostream>

int main()
{
  nom::PlatformSpec spec = nom::platform_info();
  std::cout << "CPUs: " << spec.num_cpus << std::endl;

  nom::Point2i p(10, 20);
  std::cout << "Point: (" << p.x << ", " << p.y << ")" << std::endl;

  nom::Size2i s(640, 480);
  std::cout << "Size: " << s.w << "x" << s.h << std::endl;

  nom::Color4i c(255, 128, 0, 255);
  std::cout << "Color: " << (int)c.r << "," << (int)c.g << "," << (int)c.b << "," << (int)c.a << std::endl;

  std::cout << "nomlib consumer test: OK" << std::endl;
  return 0;
}
