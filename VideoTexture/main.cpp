/*-----------------------------------------------------------------------

Matt Marchant 2024
http://trederia.blogspot.com

Video Texture for SFML - Zlib license.

This software is provided 'as-is', without any express or
implied warranty.In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.

-----------------------------------------------------------------------*/

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
    sprite01.setPosition({ 512.f, 384.f });
    sprite01.setOrigin(sf::Vector2f(videoTexture.getTexture().getSize()) / 2.f);

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