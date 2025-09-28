#include <iostream>
#include <cmath>
#include <climits>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>

//* Nlohmann for json files (Man I love this guy!)
#include <nlohmann/json.hpp>
using json = nlohmann::json;

//* Paths and Files
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

std::unordered_map<std::string, sf::Texture> textureMap;

json settings;

std::unordered_map<std::string, std::wstring> mediaInfo {
    {"Title", L""},
    {"Artist", L""},
    {"Album", L""},
};
std::unordered_map<std::string, int> durations {
    {"PositionM", 0},
    {"PositionS", 0},
    {"DurationM", 0},
    {"DurationS", 0}
};


std::string path = fs::current_path().string() + "/";

//* Window Variables
sf::RenderWindow window;

//* Render Sprite
sf::Texture tempTexture({450, 450});
sf::Sprite renderSprite(tempTexture);

//* framerate vars:
int framerate;
#include <chrono>
std::chrono::high_resolution_clock::time_point frameStart;
std::chrono::high_resolution_clock::time_point frameEnd;

//* Media Controll
#include <winrt/Windows.foundation.h>
#include <winrt/Windows.media.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::Streams;

void ToMinSec(int totalSeconds, std::string minutes, std::string seconds) {
    durations[minutes] = totalSeconds / 60;
    durations[seconds] = totalSeconds % 60;
}

void getPlaybackData() {
    auto mgr = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    auto session = mgr.GetCurrentSession();
    if (!session)
    {
        std::cerr << "No active media session found.\n";
        return;
    }

    // 4.1 Metadata (artist, title, album…)
    auto props = session.TryGetMediaPropertiesAsync().get();

    // 4.2 Timeline (position & duration)
    auto timeline = session.GetTimelineProperties();
    int posSec = std::chrono::duration_cast<std::chrono::seconds>(timeline.Position()).count();
    int durSec = std::chrono::duration_cast<std::chrono::seconds>(timeline.EndTime()).count();

    ToMinSec(posSec, "PositionM", "PositionS");
    ToMinSec(durSec, "DurationM", "DurationS");

    // 4.3 Update Metadata (artist, title, album…)
    std::wstring tempTitle = props.Title().c_str();
    if (mediaInfo["Title"] != tempTitle){
        mediaInfo["Title"]  = tempTitle;
        mediaInfo["Artist"] = props.Artist().c_str();
        mediaInfo["Album"]  = props.AlbumTitle().c_str();

        // 4.3 Cover art thumbnail
        if (auto thumb = props.Thumbnail())
        {
            auto stream = thumb.OpenReadAsync().get();
            uint64_t size = stream.Size();
            Buffer buf{ static_cast<uint32_t>(size) };
            buf.Length(static_cast<uint32_t>(size));
            stream.ReadAsync(buf, buf.Capacity(), InputStreamOptions::None).get();
    
            auto reader = DataReader::FromBuffer(buf);
            std::vector<uint8_t> bytes(buf.Length());
            reader.ReadBytes(bytes);
    
            sf::Image img;
            if (img.loadFromMemory(bytes.data(), bytes.size()))
            {
                if (textureMap["cover"].loadFromImage(img)) {
                } else {
                    std::cout << "Cover Error\n";
                }
            }
        }
    }
}

/*
* Loads A filePath into a Json Obj, passed by reference.
*/
void loadJson(json& jsonFile, const std::string pathToJson, const std::string jsonIdentifyer = "JsonName") {
    std::ifstream file(pathToJson);
    if (file.is_open()) {
        try {
            file >> jsonFile;
        } catch (json::parse_error& e) {
            std::cout << "JSON Parse Error in " << jsonIdentifyer << " : " << e.what() << "\n";
        }
    } else {
        std::cout << "Error opening and loading " << jsonIdentifyer << "\n";
    }
}

/*
* Sets the Window Icon to default icon and draws the loading logo
*/
void setUpWindowIconAndLoadingLogo() {
    //* Setting Window Icon
    sf::Image icon;
    if (!icon.loadFromFile(path+"logos/icon.png")) {
        std::cout << "RaisedError: Icon could not be loaded!\n";
    }
    window.setIcon(icon);
}

/*
 * Draw the given Text on top of the RenderTexture (Pointer), with the given Parameter
 * Orientations:
 * -1 Left
 * 0 Center
 * 1 Right
 */
void drawText(const std::wstring text, unsigned int fontSize, const sf::Vector2f pos, const sf::Color color = sf::Color::White, bool shadow = false, int orientation = -1)
{   
    sf::Font font;
    if (!font.openFromFile(path+"/font/Arial-Unicode-Regular.ttf")) {
        std::cout << "Font Not Found! Under: " + path+"/font/Arial-Unicode-Regular.ttf" + "\n";
    }
    sf::Text sfText(font, sf::String(text), fontSize);

    if (shadow)
    {
        sfText.setFillColor(sf::Color::Black);
        float shadowWidth = fontSize / 6.4;
        if (orientation == 0)
        {
            float textWidth = sfText.getLocalBounds().size.x;
            sfText.setPosition({pos.x - (textWidth / 2) + shadowWidth, pos.y + shadowWidth});
        }
        else if (orientation == 1)
        {
            float textWidth = sfText.getLocalBounds().size.x;
            sfText.setPosition({pos.x - textWidth + shadowWidth, pos.y + shadowWidth});
        }
        else
        {
            sfText.setPosition({pos.x + shadowWidth, pos.y + shadowWidth});
        }

        window.draw(sfText);
    }

    sfText.setFillColor(color);

    if (orientation == 0)
    {
        float textWidth = sfText.getLocalBounds().size.x;
        sfText.setPosition({pos.x - (textWidth / 2), pos.y});
    }
    else if (orientation == 1)
    {
        float textWidth = sfText.getLocalBounds().size.x;
        sfText.setPosition({pos.x - textWidth, pos.y});
    }
    else
    {
        sfText.setPosition(pos);
    }

    window.draw(sfText);
}

/*
 * Rendered At Pos: {5, 5}
*/
void renderFramerate()
{
    if (settings["showFramerate"])
    {
        drawText(std::to_wstring(framerate), 15, {5, 5}, sf::Color::White, true);
    }
}

int main(int argc, char* argv[]) {
    winrt::init_apartment();

    loadJson(settings, path+"/settings/settings.json", "settings");

    //* Create Window
    sf::VideoMode vMode = sf::VideoMode({450, 600});
    window.create(vMode, "MediaControllVisualizer", sf::Style::Default, sf::State::Windowed);
    sf::Vector2u wSize = {450, 600};
    window.setSize(wSize);
    window.setMinimumSize(wSize);
    window.setMaximumSize(wSize);

    //* Update Settings:
    window.setFramerateLimit(60);

    setUpWindowIconAndLoadingLogo();

    while (window.isOpen())
    {
        frameStart = std::chrono::high_resolution_clock::now();
        renderFramerate();
        
        getPlaybackData();

        while (const std::optional event = window.pollEvent())
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // Get the original size of the texture
        sf::Vector2u texSize = textureMap["cover"].getSize();
        sf::Vector2u wSize = window.getSize();
    
        // Calculate the scaling factors
        float scaleX = wSize.x / static_cast<float>(texSize.x);
        float scaleY = 450 / static_cast<float>(texSize.y);
    
        //Apply the scaling
        renderSprite.setTexture(textureMap["cover"], true);
        renderSprite.setScale({scaleX, scaleY});
    
        window.clear();
        window.draw(renderSprite);
    
        if (settings["display"]["title"].get<bool>()) {
            drawText(mediaInfo["Title"].c_str(), settings["fontSizes"]["title"].get<int>(), {225, settings["textPositionsY"]["title"].get<float>()}, sf::Color::White, true, 0);
        }
        if (settings["display"]["artist"].get<bool>()) {
            drawText(mediaInfo["Artist"].c_str(), settings["fontSizes"]["artist"].get<int>(), {225, settings["textPositionsY"]["artist"].get<float>()}, sf::Color::White, true, 0);
        }
        if (settings["display"]["album"].get<bool>()) {
            drawText(mediaInfo["Album"].c_str(), settings["fontSizes"]["album"].get<int>(), {225, settings["textPositionsY"]["album"].get<float>()}, sf::Color::White, true, 0);
        }

        
    
        if (settings["display"]["duration"].get<bool>()) {
            std::wstring timePos = std::to_wstring(durations["PositionM"]) + L":" + std::to_wstring(durations["PositionS"]);
            std::wstring timeDur = std::to_wstring(durations["DurationM"]) + L":" + std::to_wstring(durations["DurationS"]);;
            std::wstring timeBar = timePos + L"/" + timeDur + L" Seconds";
            drawText(timeBar.c_str(), settings["fontSizes"]["duration"].get<int>(), {225, settings["textPositionsY"]["duration"].get<float>()}, sf::Color::White, true, 0);
        }
        
        window.display();

        frameEnd = std::chrono::high_resolution_clock::now();
        framerate = (int)((float)1e9 / (float)std::chrono::duration_cast<std::chrono::nanoseconds>(frameEnd - frameStart).count());
        renderFramerate();
    }
    return 0;
}

