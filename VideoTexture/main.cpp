#include "src/VideoTexture.hpp"

#include <SFML/graphics.hpp>


int main()
{
    sf::RenderWindow window;
    window.create({ 1024, 768 }, "Window");

    //test video can be downloaded from the link in the pl_mpeg readme
    //https://github.com/phoboslab/pl_mpeg/blob/master/README.md

    VideoTexture videoTexture;
    if (videoTexture.loadFromFile("test.mpeg"))
    {
        videoTexture.play();
    }

    sf::Clock frameClock;

    //because this is a texture it can be used with multiple drawables
    sf::Sprite sprite01(videoTexture.getTexture());

    sf::Sprite sprite02(videoTexture.getTexture());
    sprite02.setScale({ 0.5f, 0.5f });
    sprite02.setColor(sf::Color::Magenta);

    sf::Sprite sprite03(videoTexture.getTexture());
    sprite03.setScale({ 0.5f, 0.5f });
    sprite03.setPosition({ 512.f, 384.f });

    while (window.isOpen())
    {
        sf::Event evt;
        while (window.pollEvent(evt))
        {
            if (evt.type == sf::Event::Closed)
            {
                window.close();
            }

        }

        const float dt = frameClock.restart().asSeconds();
        videoTexture.update(dt);
        sprite03.rotate(100.f * dt);


        window.clear();
        window.draw(sprite01);
        window.draw(sprite02);
        window.draw(sprite03);
        window.display();
    }

    return 0;
}