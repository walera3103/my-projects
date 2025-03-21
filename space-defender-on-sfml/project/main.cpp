#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "perfect_collision.h"

class Text {
private:
    sf::Font font;
    sf::Text game_over_text;
    sf::Text start_game_option_text;
protected:
    void fonts() {
        if(!font.loadFromFile("d9464-arkhip_font.ttf")) {
            std::cout << "error with text" << std::endl;
        }
    }
    sf::Text game_over_text_function() {
        game_over_text.setFont(font);
        game_over_text.setString("GAME OVER");
        game_over_text.setColor(sf::Color::Red);
        game_over_text.setCharacterSize(40);
        game_over_text.setPosition(sf::Vector2f(500, 300));

        return game_over_text;
    }
    sf::Text game_option_text_function(std::string text, int x, int y) {
        start_game_option_text.setFont(font);
        start_game_option_text.setString(text);
        start_game_option_text.setColor(sf::Color::Yellow);
        start_game_option_text.setCharacterSize(40);
        start_game_option_text.setPosition(sf::Vector2f(x, y));

        return start_game_option_text;
    }
};

class Music_and_sounds : protected Text {
private:
    sf::SoundBuffer buffer_for_simple_fire;
    sf::SoundBuffer buffer_for_explosion;
    sf::SoundBuffer buffer_for_hit;
    sf::SoundBuffer buffer_for_siren;
    sf::SoundBuffer buffer_for_preparing_napalm;
    sf::SoundBuffer buffer_for_preparing_laser;
    sf::SoundBuffer buffer_for_hiperjump;
    //sf::SoundBuffer buffer_for_napalm;
protected:
    sf::Sound sound_for_simple_fire;
    sf::Sound sound_for_explosion;
    sf::Sound sound_for_hit;
    sf::Sound sound_for_siren;
    sf::Sound sound_for_preparing_napalm;
    sf::Sound sound_for_preparing_laser;
    sf::Sound sound_for_hiperjump;
    //sf::Sound sound_for_napalm;

    void buffers() {
        if(!buffer_for_simple_fire.loadFromFile("laser-blast-descend_gy7c5deo(online-audio-converter.com).wav")) {
            std::cout << "/buffer for simple fire not added/" << std::endl;
        }
        if(!buffer_for_explosion.loadFromFile("explosion-blast_f1p8ro4o(online-audio-converter.com).wav")) {
            std::cout << "/buffer for explosion not added" << std::endl;
        }
        if(!buffer_for_hit.loadFromFile("explosion-metal-debris-blast_zk-60svo(online-audio-converter.com).wav")) {
            std::cout << "/buffer for hit not added/" << std::endl;
        }
        if(!buffer_for_siren.loadFromFile("mix_04s(online-audio-converter.com).wav")) {
            std::cout << "/buffer for siren not added/" << std::endl;
        }
        if(!buffer_for_preparing_napalm.loadFromFile("jg-032316-sfx-sci-fi-blaster-weapon-powerup-and-fire-4mp3cutnet_JRmkF2hW.wav")) {
            std::cout << "/buffer for preparing napalm not added/" << std::endl;
        }
        if(!buffer_for_preparing_laser.loadFromFile("jg-032316-sfx-sci-fi-blaster-weapon-powerup-and-fire-4mp3cutnet-jrmkf2hw-mp3cut_MLMLqkpj.wav")) {
            std::cout << "/buffer for preparing laser not added/" << std::endl;
        }
        if(!buffer_for_hiperjump.loadFromFile("12120(mp3cut.net).wav")) {
            std::cout << "/buffer for hiperjump not added/" << std::endl;
        }
        sound_for_simple_fire.setBuffer(buffer_for_simple_fire);
        sound_for_explosion.setBuffer(buffer_for_explosion);
        sound_for_hit.setBuffer(buffer_for_hit);
        sound_for_siren.setBuffer(buffer_for_siren);
        sound_for_preparing_napalm.setBuffer(buffer_for_preparing_napalm);
        sound_for_preparing_laser.setBuffer(buffer_for_preparing_laser);
        sound_for_hiperjump.setBuffer(buffer_for_hiperjump);
    }
};

class O_bject : protected Music_and_sounds {
private:
const int background_x = 0;
const int background_y = 0;
int player_x = 610;
int player_y = 500;
sf::Texture texture_for_background;
sf::Texture texture_for_player;
sf::Texture texture_for_simple_enemy;
sf::Texture texture_for_brain;
sf::Texture texture_for_laser_enemy;
sf::Texture texture_for_kamikaze;
sf::Texture texture_for_name;
protected:
    void inicjalisations_for_textures() {
        if(!texture_for_background.loadFromFile("mob-shutterstock-481251031-20210610135721_1920x1080(changed).jpg")) {
            std::cout << "/texture for background not added/" << std::endl;
        }
        if(!texture_for_player.loadFromFile("5.png")) {
            std::cout << "/texture for player not added/" << std::endl;
        }
        if(!texture_for_simple_enemy.loadFromFile("1.png")) {
            std::cout << "/texture for simple enemy not added/" << std::endl;
            //Collision::CreateTextureAndBitmask(texture_for_simple_enemy, "1.png");
        }
        if(!texture_for_brain.loadFromFile("6.png")) {
            std::cout << "/texture for brain not added/" << std::endl;
        }
        if(!texture_for_laser_enemy.loadFromFile("3.png")) {
            std::cout << "/texture for laser enemy not added/" << std::endl;
        }
        if(!texture_for_kamikaze.loadFromFile("4.png")) {
            std::cout << "/texture for kamikaze not added/" << std::endl;
        }
        if(!texture_for_name.loadFromFile("Artboard1(changed).png")) {
            std::cout << "/texture for name not added/" << std::endl;
        }
    }
    sf::Sprite inicjalisations_for_background() {
        sf::Sprite background(texture_for_background);
        background.setPosition(sf::Vector2f(background_x, background_y));
        return background;
    }

    sf::Sprite inicjalisations_for_player() {
        sf::Sprite player(texture_for_player);
        player.setPosition(sf::Vector2f(player_x, player_y));
        return player;
    }
    sf::Sprite inicjalisations_for_simple_enemy(int x, int y) {
        sf::Sprite simple_enemy(texture_for_simple_enemy);
        simple_enemy.setPosition(sf::Vector2f(x, y));
        return simple_enemy;
    }
    sf::Sprite inicjalisations_for_brain() {
        sf::Sprite brain(texture_for_brain);
        brain.setPosition(sf::Vector2f(-60, 150));
        return brain;
    }
    sf::Sprite inicjalisations_for_laser_enemy() {
        sf::Sprite laser_enemy(texture_for_laser_enemy);
        laser_enemy.setPosition(sf::Vector2f(1200, 150));

        return laser_enemy;
    }
    sf::Sprite inicjalisation_for_kamikaze() {
        sf::Sprite kamikaze(texture_for_kamikaze);
        kamikaze.setPosition(sf::Vector2f(640, -60));

        return kamikaze;
    }
    sf::Sprite inicjalisation_for_name() {
        sf::Sprite name(texture_for_name);
        name.setPosition(sf::Vector2f(395, -468));

        return name;
    }
    sf::CircleShape inicjalisations_for_simple_fire_for_player(int x, int y) {
        sf::CircleShape blue_player_fire(4);
        blue_player_fire.setFillColor(sf::Color::Green);
        blue_player_fire.setPosition(sf::Vector2f(x, y));

        return blue_player_fire;
    }
    sf::CircleShape inicjalisation_for_napalm(int x, int y) {
        sf::CircleShape napalm(4);
        napalm.setFillColor(sf::Color::Red);
        napalm.setPosition(sf::Vector2f(x, y));

        return napalm;
    }
    sf::CircleShape inicjalisation_for_preparing(int x, int y) {
        sf::CircleShape prepare_for_napalm(8);
        prepare_for_napalm.setFillColor(sf::Color::Red);
        prepare_for_napalm.setPosition(sf::Vector2f(x, y));

        return prepare_for_napalm;
    }
    sf::RectangleShape inicjalisation_for_laser(int x, int y) {
        sf::RectangleShape laser(sf::Vector2f(4, 600));
        laser.setPosition(x, y);
        laser.setFillColor(sf::Color::Red);

        return laser;
    }
    sf::RectangleShape inicjalisation_for_options_for_menu(int x, int y) {
        sf::RectangleShape option(sf::Vector2f(300, 100));
        option.setPosition(x, y);
        option.setFillColor(sf::Color::Black);
        //start_game_option_text.setFont(font);
        //sf::Text start_game_option = start_game_option_text_function(text);

        return option;
    }
};

class Game : public O_bject{
private:
    const int window_height = 600;
    const int window_width = 1280;
    const int deviation_for_left_player_gun_x = 5;
    const int deviation_for_left_player_gun_y = 20;
    const int deviation_for_right_player_gun_x = 45;
    const float speed = 0.5;
    std::vector<sf::CircleShape> player_shots;
    std::vector<sf::Sprite> simple_enemyes;
    std::vector<sf::CircleShape> simple_enemy_shots;
    std::vector<sf::Sprite> brains;
    std::vector<sf::CircleShape> prepearing_for_napalm;
    std::vector<sf::CircleShape> napalm_shots;
    std::vector<sf::Sprite> laser_enemy;
    std::vector<sf::CircleShape> preparing_for_laser;
    std::vector<sf::RectangleShape> lasers;
    std::vector<int> brains_height;
    std::vector<int> coefficient_for_brains;
    std::vector<int> time_to_napalm;
    std::vector<int> actualy_time_to_napalm;
    std::vector<int> time_to_prepearing_napalm;
    std::vector<int> timer_for_brain_shots;
    std::vector<int> laser_enemy_height;
    std::vector<int> coefficient_for_laser_enemy;
    std::vector<int> time_to_laser;
    std::vector<int> actualy_time_to_laser;
    std::vector<int> time_to_preparing_laser;
    std::vector<int> timer_for_laser_shots;
    std::vector<int> is_brain_is_still_alive;
    std::vector<int> is_laser_enemy_still_alive;
    std::vector<bool> is_brains_going_up;
    std::vector<bool> is_napalm_going_on;
    std::vector<bool> is_brain_here;
    std::vector<bool> is_laser_enemy_going_up;
    std::vector<bool> is_laser_going_on;
    std::vector<bool> is_laser_enemy_here;
    sf::Clock clock;
    bool is_this_second_wave = false;
    bool is_kamikaze_here = false;
    bool is_player_alive = true;
    bool is_second_wave_lose = false;
    float move_for_player_x = 0;
    float move_for_player_y = 0;
    int player_position_x, player_position_y;
    int frequency_for_simple_fire = 0;
    int frequency_for_simple_enemy_fire = 120;
    int frequency_for_brains = 0;
    int frequency_for_laser_enemy = 720;
    int number_of_simple_enemyes = 30;
    int rand_for_simple_enemy_fire;
    int player_hp = 4;
    int still_alive_enemys_hp = 90;
    int timer_for_kamikaze = 0;
    int coefficient_for_kamikaze_x;
    int coefficient_for_kamikaze_y;
    int kamikaze_hp = 2;
    int timer_for_hyperjump = 0;
    int transparency = 0;
    int player_choice = 1;
    int timer_to_return_to_the_menu = 0;
    int is_simple_enemy_is_still_alive[30];

    void prepare_levels() {
        player_shots.clear();
        simple_enemyes.clear();
        simple_enemy_shots.clear();
        brains.clear();
        prepearing_for_napalm.clear();
        napalm_shots.clear();
        laser_enemy.clear();
        preparing_for_laser.clear();
        lasers.clear();
        brains_height.clear();
        coefficient_for_brains.clear();
        time_to_napalm.clear();
        actualy_time_to_napalm.clear();
        time_to_prepearing_napalm.clear();
        timer_for_brain_shots.clear();
        laser_enemy_height.clear();
        coefficient_for_laser_enemy.clear();
        time_to_laser.clear();
        actualy_time_to_laser.clear();
        time_to_preparing_laser.clear();
        timer_for_laser_shots.clear();
        is_brain_is_still_alive.clear();
        is_laser_enemy_still_alive.clear();
        is_brains_going_up.clear();
        is_napalm_going_on.clear();
        is_brain_here.clear();
        is_laser_enemy_going_up.clear();
        is_laser_going_on.clear();
        is_laser_enemy_here.clear();
        is_this_second_wave = false;
        is_kamikaze_here = false;
        is_player_alive = true;
        is_second_wave_lose = false;
        move_for_player_x = 0;
        move_for_player_y = 0;
        frequency_for_simple_fire = 0;
        frequency_for_simple_enemy_fire = 120;
        frequency_for_brains = 0;
        frequency_for_laser_enemy = 720;
        number_of_simple_enemyes = 30;
        rand_for_simple_enemy_fire;
        player_hp = 4;
        still_alive_enemys_hp = 90;
        timer_for_kamikaze =
        kamikaze_hp = 2;
        timer_for_hyperjump = 0;
        timer_to_return_to_the_menu = 0;
    }
    void draw_simple_enemyes() {
        simple_enemyes.clear();
        for(int i = 0; i < number_of_simple_enemyes; i++) {
            if(i <= 9){
                simple_enemyes.push_back(inicjalisations_for_simple_enemy(190 + i * 90, -210));
            }
            else if(i >= 10 && i <= 19) {
                simple_enemyes.push_back(inicjalisations_for_simple_enemy(190 + ((i - 10) * 90), -140));
            }
            else if(i >= 20 && i <= 29) {
                simple_enemyes.push_back(inicjalisations_for_simple_enemy(190 + ((i - 20) * 90), -70));
           }
        }
    }

    void give_hp_for_simple_enemyes() {
        for(int i = 0; i < number_of_simple_enemyes; i++) {
            is_simple_enemy_is_still_alive[i] = 0;
        }
    }

    void registration_of_hits_for_player(sf::Sprite player) {
        player_hp--;
        if(player_hp >= 0) {
            sound_for_hit.play();
        }
        if(player_hp == 1) {
            sound_for_siren.play();
        }
        if(player_hp == 0) {
            player.setPosition(sf::Vector2f(-10, -10));
            sound_for_siren.stop();
            is_player_alive = false;
        }
    }
    void brains_dead(int value) {
        sound_for_preparing_napalm.stop();
        brains.erase(brains.begin() + value);
        brains_height.erase(brains_height.begin() + value);
        coefficient_for_brains.erase(coefficient_for_brains.begin() + value);
        is_brains_going_up.erase(is_brains_going_up.begin() + value);
        time_to_napalm.erase(time_to_napalm.begin() + value);
        actualy_time_to_napalm.erase(actualy_time_to_napalm.begin() + value);
        is_napalm_going_on.erase(is_napalm_going_on.begin() + value);
        time_to_prepearing_napalm.erase(time_to_prepearing_napalm.begin() + value);
        is_brain_here.erase(is_brain_here.begin() + value);
        timer_for_brain_shots.erase(timer_for_brain_shots.begin() + value);
        is_brain_is_still_alive.erase(is_brain_is_still_alive.begin() + value);
    }
    void laser_enemy_dead(int value) {
        sound_for_preparing_laser.stop();
        laser_enemy.erase(laser_enemy.begin() + value);
        laser_enemy_height.erase(laser_enemy_height.begin() + value);
        coefficient_for_laser_enemy.erase(coefficient_for_laser_enemy.begin() + value);
        is_laser_enemy_going_up.erase(is_laser_enemy_going_up.begin() + value);
        time_to_laser.erase(time_to_laser.begin() + value);
        actualy_time_to_laser.erase(actualy_time_to_laser.begin() + value);
        is_laser_going_on.erase(is_laser_going_on.begin() + value);
        time_to_preparing_laser.erase(time_to_preparing_laser.begin() + value);
        is_laser_enemy_here.erase(is_laser_enemy_here.begin() + value);
        timer_for_laser_shots.erase(timer_for_laser_shots.begin() + value);
        is_laser_enemy_still_alive.erase(is_laser_enemy_still_alive.begin() + value);
    }

public:
    void draw_first_or_second_level() {
        sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Space Defender");
        window.setMouseCursorVisible(false);
        window.setFramerateLimit(120);
        srand(time(NULL));

        inicjalisations_for_textures();
        buffers();
        fonts();
        prepare_levels();
        sf::Sprite background = inicjalisations_for_background();
        sf::Sprite player = inicjalisations_for_player();
        sf::Sprite kamikaze = inicjalisation_for_kamikaze();
        sf::Text game_over_text = game_over_text_function();


        give_hp_for_simple_enemyes();
        draw_simple_enemyes();
        while(window.isOpen()) {
            sf::Event event;
            auto player_bound = player.getGlobalBounds();

            while(window.pollEvent(event)) {
                switch(event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;

                }

            }
            sf::Time time = clock.getElapsedTime();
            clock.restart().asMilliseconds();
            player_position_x = player.getPosition().x;
            player_position_y = player.getPosition().y;
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                if((player_position_x - speed) >= 0 && player_hp > 0 && (is_second_wave_lose == false || brains.size() > 0 || laser_enemy.size() > 0))
                move_for_player_x = -speed;
            }
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                if((player_position_x + speed + 60) <= window_width && player_hp > 0 && (is_second_wave_lose == false || brains.size() > 0 || laser_enemy.size() > 0))
                move_for_player_x = speed;
            }
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                if((player_position_y - speed) >= 0 && player_hp > 0 && (is_second_wave_lose == false || brains.size() > 0 || laser_enemy.size() > 0))
                move_for_player_y = -speed;
            }
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                if((player_position_y + speed + 60) <= window_height && player_hp > 0 && (is_second_wave_lose == false || brains.size() > 0 || laser_enemy.size() > 0))
                move_for_player_y = speed;
            }
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && is_player_alive == true && (is_second_wave_lose == false || brains.size() > 0 || laser_enemy.size() > 0)) {
                if(frequency_for_simple_fire == 0) {
                    player_shots.push_back(inicjalisations_for_simple_fire_for_player(player_position_x + deviation_for_left_player_gun_x, player_position_y + deviation_for_left_player_gun_y));
                    //player_shots.end()->getGlobalBounds();
                    player_shots.push_back(inicjalisations_for_simple_fire_for_player(player_position_x + deviation_for_left_player_gun_x + deviation_for_right_player_gun_x, player_position_y + deviation_for_left_player_gun_y));
                    frequency_for_simple_fire = 25;
                    sound_for_simple_fire.play();
                }
                else
                {
                    frequency_for_simple_fire--;
                }
            }

            player.move(sf::Vector2f(move_for_player_x * time.asMilliseconds(), move_for_player_y * time.asMilliseconds()));
            move_for_player_x = 0;
            move_for_player_y = 0;

            if(frequency_for_simple_enemy_fire == 0) {
                while(true) {
                    if(still_alive_enemys_hp == 0) {
                        break;
                    }
                    rand_for_simple_enemy_fire = rand() % 30;
                    if(is_simple_enemy_is_still_alive[rand_for_simple_enemy_fire] != 3) {
                        simple_enemy_shots.push_back(inicjalisations_for_simple_fire_for_player(simple_enemyes[rand_for_simple_enemy_fire].getPosition().x + 30, simple_enemyes[rand_for_simple_enemy_fire].getPosition().y));
                        break;
                    }
                }
                frequency_for_simple_enemy_fire = 60;
            }
            else {
                frequency_for_simple_enemy_fire--;
            }
            if(still_alive_enemys_hp == 0 && is_this_second_wave == false) {
                still_alive_enemys_hp = 90;
                is_this_second_wave = true;
                draw_simple_enemyes();
                give_hp_for_simple_enemyes();
            }
            if(still_alive_enemys_hp == 0 && is_this_second_wave == true) {
                is_second_wave_lose = true;
            }

            if(frequency_for_brains == 0 && brains.size() < 3 && is_second_wave_lose == false) {
                frequency_for_brains = 960;
                brains.push_back(inicjalisations_for_brain());
                brains_height.push_back(0);
                coefficient_for_brains.push_back(1);
                is_brains_going_up.push_back(true);
                time_to_napalm.push_back(0);
                actualy_time_to_napalm.push_back(0);
                is_napalm_going_on.push_back(false);
                time_to_prepearing_napalm.push_back(240);
                is_brain_here.push_back(true);
                timer_for_brain_shots.push_back(240);
                is_brain_is_still_alive.push_back(0);

            }
            else {
                frequency_for_brains--;
            }
            prepearing_for_napalm.clear();
            for(int i = 0; i < brains.size(); i++) {
                prepearing_for_napalm.push_back(inicjalisation_for_preparing(brains[i].getPosition().x + 30, brains[i].getPosition().y + 60));
            }
            for(int i = 0; i < brains.size(); i++) {
                if(is_brains_going_up[i]) {
                    brains_height[i]--;
                    if(brains_height[i] == -30) {
                        is_brains_going_up[i] = false;
                    }
                    brains[i].move(sf::Vector2f(coefficient_for_brains[i] * ((speed * time.asMilliseconds()) / 2), -(speed * time.asMilliseconds() / 3)));
                }
                else {
                    brains_height[i]++;
                    if(brains_height[i] == 60) {
                        is_brains_going_up[i] = true;
                    }
                    brains[i].move(sf::Vector2f(coefficient_for_brains[i] * ((speed * time.asMilliseconds()) / 2), speed * time.asMilliseconds() / 3));
                }
                if((brains[i].getPosition().x + speed / 2 + 60) > window_width) {
                    coefficient_for_brains[i] = -1;
                }
                if((brains[i].getPosition().x - speed / 2) < 0) {
                    coefficient_for_brains[i] = 1;
                }
                if(!is_napalm_going_on[i]) {
                    if(time_to_napalm[i] == 0) {
                        time_to_napalm[i] = (rand() % 5 + 1) * 120;
                        actualy_time_to_napalm[i] = 0;
                    }
                    else {
                        actualy_time_to_napalm[i]++;
                        if(actualy_time_to_napalm[i] == time_to_napalm[i]) {
                            sound_for_preparing_napalm.play();
                            timer_for_brain_shots[i] = 0;
                            time_to_napalm[i] = 0;
                            is_napalm_going_on[i] = true;
                        }
                    }
                }
                if(timer_for_brain_shots[i] < 240 && is_napalm_going_on[i] == false) {
                    if(timer_for_brain_shots[i] % 6 == 0) {
                        napalm_shots.push_back(inicjalisation_for_napalm(brains[i].getPosition().x + 30, brains[i].getPosition().y + 60));
                    }
                    timer_for_brain_shots[i]++;
                }
            }

            for(int i = 0; i < napalm_shots.size(); i++) {
                napalm_shots[i].move(sf::Vector2f(0, speed * time.asMilliseconds()));
                if(napalm_shots[i].getPosition().y > 610) {
                    napalm_shots.erase(napalm_shots.begin());
                }
                if(player_bound.intersects(napalm_shots[i].getGlobalBounds())) {
                    napalm_shots.erase(napalm_shots.begin() + i);
                    registration_of_hits_for_player(player);
                }
            }

            if(frequency_for_laser_enemy == 0 && is_second_wave_lose == false) {
                frequency_for_laser_enemy = 720;
                laser_enemy.push_back(inicjalisations_for_laser_enemy());
                laser_enemy_height.push_back(0);
                coefficient_for_laser_enemy.push_back(-1);
                is_laser_enemy_going_up.push_back(false);
                time_to_laser.push_back(0);
                actualy_time_to_laser.push_back(0);
                is_laser_going_on.push_back(false);
                time_to_preparing_laser.push_back(240);
                is_laser_enemy_here.push_back(true);
                timer_for_laser_shots.push_back(240);
                is_laser_enemy_still_alive.push_back(0);
            }
            else {
                frequency_for_laser_enemy--;
            }
            preparing_for_laser.clear();
            lasers.clear();
            for(int i = 0; i < laser_enemy.size(); i++) {
                if(coefficient_for_laser_enemy[i] == 1) {
                    preparing_for_laser.push_back(inicjalisation_for_preparing(laser_enemy[i].getPosition().x + 28, laser_enemy[i].getPosition().y + 30));
                    if(timer_for_laser_shots[i] < 200 && is_laser_going_on[i] == false) {
                        timer_for_laser_shots[i]++;
                        lasers.push_back(inicjalisation_for_laser(laser_enemy[i].getPosition().x + 32, laser_enemy[i].getPosition().y + 30));
                    }
                }
                else {
                    preparing_for_laser.push_back(inicjalisation_for_preparing(laser_enemy[i].getPosition().x + 22, laser_enemy[i].getPosition().y + 30));
                    if(timer_for_laser_shots[i] < 200 && is_laser_going_on[i] == false) {
                        timer_for_laser_shots[i]++;
                        lasers.push_back(inicjalisation_for_laser(laser_enemy[i].getPosition().x + 28, laser_enemy[i].getPosition().y + 30));
                    }
                }
            }
            for(int i = 0; i < laser_enemy.size(); i++) {
                if(is_laser_enemy_going_up[i]) {
                    laser_enemy_height[i]--;
                    if(laser_enemy_height[i] == -30) {
                        is_laser_enemy_going_up[i] = false;
                    }
                    laser_enemy[i].move(sf::Vector2f(coefficient_for_laser_enemy[i] * ((speed * time.asMilliseconds()) / 2), -(speed * time.asMilliseconds() / 3)));
                }
                else {
                    laser_enemy_height[i]++;
                    if(laser_enemy_height[i] == 60) {
                        is_laser_enemy_going_up[i] = true;
                    }
                    laser_enemy[i].move(sf::Vector2f(coefficient_for_laser_enemy[i] * ((speed * time.asMilliseconds()) / 2), speed * time.asMilliseconds() / 3));
                }
                if((laser_enemy[i].getPosition().x + speed / 2 + 60) > window_width) {
                    coefficient_for_laser_enemy[i] = -1;
                }
                if((laser_enemy[i].getPosition().x - speed / 2) < 0) {
                    coefficient_for_laser_enemy[i] = 1;
                }
                if(!is_laser_going_on[i]) {
                    if(time_to_laser[i] == 0) {
                        time_to_laser[i] = (rand() % 5 + 1) * 120;
                        actualy_time_to_laser[i] = 0;
                    }
                    else {
                        actualy_time_to_laser[i]++;
                        if(actualy_time_to_laser[i] == time_to_laser[i]) {
                            timer_for_laser_shots[i] = 0;
                            time_to_laser[i] = 0;
                            is_laser_going_on[i] = true;
                            sound_for_preparing_laser.play();
                        }
                    }
                }
            }

            if(!is_kamikaze_here) {
                timer_for_kamikaze++;
            }
            else {
                if(player.getPosition().x < kamikaze.getPosition().x && player.getPosition().y > kamikaze.getPosition().y) {
                    coefficient_for_kamikaze_x = -1;
                    coefficient_for_kamikaze_y = 1;
                }
                else if(player.getPosition().x < kamikaze.getPosition().x && player.getPosition().y < kamikaze.getPosition().y) {
                    coefficient_for_kamikaze_x = -1;
                    coefficient_for_kamikaze_y = -1;
                }
                else if(player.getPosition().x > kamikaze.getPosition().x && player.getPosition().y > kamikaze.getPosition().y) {
                    coefficient_for_kamikaze_x = 1;
                    coefficient_for_kamikaze_y = 1;
                }
                else if(player.getPosition().x > kamikaze.getPosition().x && player.getPosition().y < kamikaze.getPosition().y) {
                    coefficient_for_kamikaze_x = 1;
                    coefficient_for_kamikaze_y = -1;
                }
                if(player.getPosition().x == kamikaze.getPosition().x) {
                    coefficient_for_kamikaze_x = 0;
                }
                if(player.getPosition().y == kamikaze.getPosition().y) {
                    coefficient_for_kamikaze_y = 0;
                }
                kamikaze.move(sf::Vector2f(coefficient_for_kamikaze_x * speed * time.asMilliseconds() / 1.5, coefficient_for_kamikaze_y * speed * time.asMilliseconds() / 3));
            }
            if(timer_for_kamikaze == 1200 && is_player_alive == true && is_this_second_wave == false) {
                timer_for_kamikaze = 0;
                is_kamikaze_here = true;
            }
            if(Collision::PixelPerfectTest(player, kamikaze)) {
                player_hp = 0;
                is_kamikaze_here = false;
                kamikaze.setPosition(sf::Vector2f(640, -60));
                is_player_alive = false;
                sound_for_hit.play();
            }

            for(int i = 0; i < lasers.size(); i++) {
                if(player_bound.intersects(lasers[i].getGlobalBounds())) {
                    registration_of_hits_for_player(player);
                }
            }

            for(int i = 0; i < simple_enemy_shots.size(); i++) {
                simple_enemy_shots[i].move(sf::Vector2f(0, speed * time.asMilliseconds()));
                if(simple_enemy_shots[i].getPosition().y > 610) {
                    simple_enemy_shots.erase(simple_enemy_shots.begin());
                }
                if(player_bound.intersects(simple_enemy_shots[i].getGlobalBounds())) {
                    simple_enemy_shots.erase(simple_enemy_shots.begin() + i);
                    registration_of_hits_for_player(player);
                }
            }

            for(int i = 0; i < player_shots.size(); i++) {
                player_shots[i].move(sf::Vector2f(0, -speed * time.asMilliseconds()));
                if(player_shots[0].getPosition().y < -10) {
                    player_shots.erase(player_shots.begin());
                }
                for(int j = 0; j < number_of_simple_enemyes; j++){
                    if(is_simple_enemy_is_still_alive[j] < 3) {
                        if(simple_enemyes[j].getGlobalBounds().intersects(player_shots[i].getGlobalBounds())) {
                            player_shots.erase(player_shots.begin() + i);
                            is_simple_enemy_is_still_alive[j]++;
                            still_alive_enemys_hp--;
                            if(is_simple_enemy_is_still_alive[j] == 3) {
                                sound_for_explosion.play();
                            }
                        }
                    }
                }
                for(int j = 0; j < brains.size(); j++) {
                    if(is_brain_is_still_alive[j] < 3) {
                        if(brains[j].getGlobalBounds().intersects(player_shots[i].getGlobalBounds())) {
                            player_shots.erase(player_shots.begin() + i);
                            is_brain_is_still_alive[j]++;
                            if(is_brain_is_still_alive[j] == 3) {
                                sound_for_explosion.play();
                                brains_dead(j);
                            }
                        }
                    }
                }
                for(int j = 0; j < laser_enemy.size(); j++) {
                    if(is_laser_enemy_still_alive[j] < 3) {
                        if(laser_enemy[j].getGlobalBounds().intersects(player_shots[i].getGlobalBounds())) {
                            player_shots.erase(player_shots.begin() + i);
                            is_laser_enemy_still_alive[j]++;
                            if(is_laser_enemy_still_alive[j] == 3) {
                                sound_for_explosion.play();
                                laser_enemy_dead(j);
                            }
                        }
                    }
                }
                if(kamikaze.getGlobalBounds().intersects(player_shots[i].getGlobalBounds()) && kamikaze.getPosition().y >= 0) {
                    kamikaze_hp--;
                    player_shots.erase(player_shots.begin() + i);
                    if(kamikaze_hp == 0) {
                        sound_for_explosion.play();
                        kamikaze_hp = 2;
                        kamikaze.setPosition(sf::Vector2f(640, -60));
                        is_kamikaze_here = false;
                    }
                }
            }
            for(int i = 0; i < 30; i++) {
                if(Collision::PixelPerfectTest(simple_enemyes[i], player) == true && is_simple_enemy_is_still_alive[i] < 3) {
                    player_hp = 0;
                    is_player_alive = false;
                    is_simple_enemy_is_still_alive[i] = 3;
                    sound_for_hit.play();
                }
            }
            for(int i = 0; i < brains.size(); i++) {
                if(Collision::PixelPerfectTest(brains[i], player) == true && is_player_alive == true) {
                    player_hp = 0;
                    is_player_alive = false;
                    brains_dead(i);
                    sound_for_hit.play();
                }
            }
            for(int i = 0; i < laser_enemy.size(); i++) {
                if(Collision::PixelPerfectTest(laser_enemy[i], player) == true && is_player_alive == true) {
                    player_hp = 0;
                    is_player_alive = false;
                    laser_enemy_dead(i);
                    sound_for_hit.play();
                }
            }

            if(is_player_alive == true && brains.size() == 0 && laser_enemy.size() == 0 && is_second_wave_lose == true) {
                if(timer_for_hyperjump == 0) {
                    sound_for_hiperjump.play();
                }
                timer_for_hyperjump++;
                if(timer_for_hyperjump >= 240) {
                    player.move(sf::Vector2f(0, -2 * speed * time.asMilliseconds()));
                }
                if(player_position_y < -30) {
                    timer_to_return_to_the_menu++;
                    if(timer_to_return_to_the_menu == 600) {
                        break;
                    }
                }
            }
            if(!is_player_alive) {
                timer_to_return_to_the_menu++;
                if(timer_to_return_to_the_menu == 600) {
                    sound_for_preparing_laser.stop();
                    sound_for_preparing_napalm.stop();
                    break;
                }
            }

            window.clear(sf::Color::Black);

            window.draw(background);

            for(int i = 0; i < simple_enemy_shots.size(); i++) {
                window.draw(simple_enemy_shots[i]);
            }
            if(player_hp > 0) {
                window.draw(player);
            }
            for(int i = 0; i < player_shots.size(); i++) {
                window.draw(player_shots[i]);
            }
            for(int i = 0; i < number_of_simple_enemyes; i++) {
                if(is_simple_enemy_is_still_alive[i] < 3) {
                    window.draw(simple_enemyes[i]);
                }
                if(simple_enemyes[29].getPosition().y <= 140) {
                    simple_enemyes[i].move(sf::Vector2f(0, speed * time.asMilliseconds()/5));
                }
            }
            for(int i = 0; i < brains.size(); i++) {
                window.draw(brains[i]);
                if(is_napalm_going_on[i]) {
                    if(time_to_prepearing_napalm[i] > 0) {
                        time_to_prepearing_napalm[i]--;
                        window.draw(prepearing_for_napalm[i]);
                    }
                    else {
                        is_napalm_going_on[i] = false;
                        time_to_prepearing_napalm[i] = 240;
                    }
                }
            }
            for(int i = 0; i < laser_enemy.size(); i++) {
                window.draw(laser_enemy[i]);
                if(is_laser_going_on[i]) {
                    if(time_to_preparing_laser[i] > 0) {
                        time_to_preparing_laser[i]--;
                        window.draw(preparing_for_laser[i]);
                    }
                    else {
                        is_laser_going_on[i] = false;
                        time_to_preparing_laser[i] = 240;
                    }
                }
            }
            for(int i = 0; i < napalm_shots.size(); i++) {
                window.draw(napalm_shots[i]);
            }
            for(int i = 0; i < lasers.size(); i++) {
                window.draw(lasers[i]);
            }
            window.draw(kamikaze);
            if(!is_player_alive) {
                window.draw(game_over_text);
            }
            window.display();

        }
    }
    void draw_menu() {
        sf::RenderWindow window(sf::VideoMode(window_width, window_height), "Space Defender");
        window.setMouseCursorVisible(false);
        window.setFramerateLimit(120);
        srand(time(NULL));

        inicjalisations_for_textures();
        buffers();
        fonts();
        sf::Sprite background = inicjalisations_for_background();
        sf::Sprite name = inicjalisation_for_name();
        sf::RectangleShape start_game_option = inicjalisation_for_options_for_menu(-300, 200);
        sf::RectangleShape exit = inicjalisation_for_options_for_menu(-300, 320);
        sf::Text start_game_text = game_option_text_function("START GAME", -295, 225);
        sf::Text exit_text = game_option_text_function("EXIT", -200, 345);

        while(window.isOpen()) {
            sf::Event event;
            sf::Time time = clock.getElapsedTime();
            clock.restart().asMilliseconds();
            while(window.pollEvent(event)) {
                switch(event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                    case sf::Keyboard::Down:
                        if(player_choice == 1) {
                            player_choice++;
                        }
                        else {
                            player_choice--;
                        }
                        break;
                    case sf::Keyboard::Up:
                        if(player_choice == 1) {
                            player_choice++;
                        }
                        else {
                            player_choice--;
                        }
                        break;
                    case sf::Keyboard::Enter:
                        if(player_choice == 1) {
                            draw_first_or_second_level();
                        }
                        else {
                            window.close();
                        }
                    }
                    break;
                }
            }

            if(transparency < 255) {
                transparency++;
            }
            background.setColor(sf::Color(255, 255, 255, transparency));
            if(transparency == 255 && name.getPosition().y < -100) {
                name.move(sf::Vector2f(0, speed * time.asMilliseconds() / 2));
            }
            if(transparency == 255 && start_game_option.getPosition().x < 490) {
                start_game_option.move(sf::Vector2f(speed * time.asMilliseconds(), 0));
                exit.move(sf::Vector2f(speed * time.asMilliseconds(), 0));
                start_game_text.move(sf::Vector2f(speed * time.asMilliseconds(), 0));
                exit_text.move(sf::Vector2f(speed * time.asMilliseconds(), 0));
            }
            if(player_choice == 1) {
                start_game_option.setFillColor(sf::Color::Blue);
                exit.setFillColor(sf::Color::Black);
            }
            else {
                start_game_option.setFillColor(sf::Color::Black);
                exit.setFillColor(sf::Color::Blue);
            }

            window.clear(sf::Color::Black);
            window.draw(background);
            window.draw(name);
            window.draw(start_game_option);
            window.draw(start_game_text);
            window.draw(exit);
            window.draw(exit_text);
            window.display();
        }
    }
};

int main() {

    Game game;
    game.draw_menu();

    return 0;
}
