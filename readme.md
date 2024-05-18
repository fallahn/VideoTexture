VideoTexture class for SFML
---------------------------

Example for using [pl_mpeg](https://github.com/phoboslab/pl_mpeg) to decode mpg1 video and audio, and render it to a texture for use with SFML drawables.



######Usage
Add the files in `src` to your SFML project. tHen do something like:

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

        sf::Sprite sprite01(videoTexture.getTexture());


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



            window.clear();
            window.draw(sprite01);
            window.display();
        }

        return 0;
    }


VideoTexture class includes api for play/pause/stop playback as well as retreiving video duration and seeking to specific points. See `VideoTexture.hpp` for details.