#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <thread>
#include <omp.h>

#include "rtweekend.h"
#include "color.h"
#include "hittable_list.h"
#include "hittable.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"



using namespace std ;



// Here we're forming a linear blend
color ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered
    if(depth <= 0)
      return color(0,0,0);
    
    if (world.hit(r, 0.001, infinity, rec)) {
      ray scattered;
      color attenuation;
      if(rec.mat_ptr->scatter(r,rec,attenuation,scattered))
          return attenuation * ray_color(scattered, world , depth-1);
      return color(0,0,0);
        
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
}


/*-----------------------------------------------------------
double random_double(){
  //Returns a random real in [0,1)
  return rand() / (RAND_MAX + 1.0);
}

double random_double(double min, double max){
  //Returns a random real in [min,max)
  return min + (max-min) * random_double();
}


---------------------------------------------------------------*/


hittable_list random_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = vec3(random_double(), random_double(), random_double()) * vec3(random_double(), random_double(), random_double()) ;
                    //auto albedo = color(0,0,0);
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dialectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dialectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}



int main(){


    // Image
    const auto aspect_ratio = 3.0/2.0; //we use a 16/9 ratio because it's common
    const int image_width     = 400;
    const int image_height    = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 10;
    const int max_depth = 50;

    // Window creation
    sf::RenderWindow window(sf::VideoMode(image_width, image_height), "Raytracer");
    window.setFramerateLimit(20);

    sf::Image image;
    image.create(image_width, image_height, sf::Color(0, 0, 0));
    sf::Texture texture;
    sf::Sprite sprite;

    
    // World

    auto world = random_scene();
    
    // Camera

    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

    // Render

    vector<color>buffer(image_width * image_height);

#pragma omp parallel for num_threads(std::thread::hardware_concurrency()) schedule(dynamic)
    for (int j = 0; j < image_height; j++)
    {
        for (int i = 0; i < image_width; ++i)
        {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }

            //write_color(cout, pixel_color, samples_per_pixel);
            auto r = pixel_color.x();
            auto g = pixel_color.y();
            auto b = pixel_color.z();

            //Divide the color by the number of samples.
            auto scale = 1.0 / samples_per_pixel;
            r = sqrt(scale * r);
            g = sqrt(scale * g);
            b = sqrt(scale * b);

            // Write the translated [0,255] value of each color component.
            r = static_cast<int>(256 * clamp(r, 0.0, 0.999));
            g = static_cast<int>(256 * clamp(g, 0.0, 0.999));
            b = static_cast<int>(256 * clamp(b, 0.0, 0.999));

           sf::Color c = sf::Color(r, g, b);
           image.setPixel(i, j, c);
        }
    }
    texture.loadFromImage(image);
    sprite.setTexture(texture);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);
    window.display();
    }
}
