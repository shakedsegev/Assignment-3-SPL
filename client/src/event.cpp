#include "../include/event.h"
#include "../include/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
using json = nlohmann::json;

Event::Event(std::string user_name, std::string team_a_name, std::string team_b_name, std::string name, int time,
             std::map<std::string, std::string> game_updates, std::map<std::string, std::string> team_a_updates,
             std::map<std::string, std::string> team_b_updates, std::string discription)
    : user_name(user_name), team_a_name(team_a_name), team_b_name(team_b_name), name(name),
      time(time), game_updates(game_updates), team_a_updates(team_a_updates),
      team_b_updates(team_b_updates), description(discription)
{
}

Event::Event(const std::string& frame_body) : user_name(""), team_a_name(""), team_b_name(""), name(""), time(0), game_updates(), team_a_updates(), team_b_updates(), description(""){
    std::stringstream stringstream(frame_body);
    std::string line;
    
    // Prefixes for extracting fields
    const std::string P_USER = "user: ";
    const std::string P_TEAM_A = "team a: ";
    const std::string P_TEAM_B = "team b: ";
    const std::string P_EVENT = "event name: ";
    const std::string P_TIME = "time: ";


    std::string current_section = "";

    while (std::getline(stringstream, line)) {
        line = trim(line);
        if (line.empty()) continue;

        // Extracting main fields
        if (line.find(P_USER) == 0) user_name = extract_value(line, P_USER);
        else if (line.find(P_TEAM_A) == 0) team_a_name = extract_value(line, P_TEAM_A);
        else if (line.find(P_TEAM_B) == 0) team_b_name = extract_value(line, P_TEAM_B);
        else if (line.find(P_EVENT) == 0) name = extract_value(line, P_EVENT);
        else if (line.find(P_TIME) == 0) time = std::stoi(extract_value(line, P_TIME));

        // Identify section transitions 
        else if (line == "general game updates:") current_section = "general";
        else if (line == "team a updates:") current_section = "team_a";
        else if (line == "team b updates:") current_section = "team_b";
        else if (line == "description:") current_section = "description";
        
        else {
            if (current_section == "description") {
                description += line + "\n";
            } else {
                // Parsing key-value pairs within statistical sections
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = trim(line.substr(0, colon_pos));
                    std::string val = trim(line.substr(colon_pos + 1));

                    if (current_section == "general") game_updates[key] = val;
                    else if (current_section == "team_a") team_a_updates[key] = val;
                    else if (current_section == "team_b") team_b_updates[key] = val;
                }
            }
        }
    }
}


Event::~Event()
{
}

const std::string &Event::get_team_a_name() const
{
    return this->team_a_name;
}

const std::string &Event::get_team_b_name() const
{
    return this->team_b_name;
}

const std::string &Event::get_name() const
{
    return this->name;
}

int Event::get_time() const
{
    return this->time;
}

const std::map<std::string, std::string> &Event::get_game_updates() const
{
    return this->game_updates;
}

const std::map<std::string, std::string> &Event::get_team_a_updates() const
{
    return this->team_a_updates;
}

const std::map<std::string, std::string> &Event::get_team_b_updates() const
{
    return this->team_b_updates;
}

const std::string &Event::get_discription() const
{
    return this->description;
}

Event::Event(const std::string &frame_body) : team_a_name(""), team_b_name(""), name(""), time(0), game_updates(), team_a_updates(), team_b_updates(), description("")
{
}

names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string team_a_name = data["team a"];
    std::string team_b_name = data["team b"];

    // run over all the events and convert them to Event objects
    std::vector<Event> events;
    for (auto &event : data["events"])
    {
        std::string name = event["event name"];
        int time = event["time"];
        std::string description = event["description"];
        std::map<std::string, std::string> game_updates;
        std::map<std::string, std::string> team_a_updates;
        std::map<std::string, std::string> team_b_updates;
        for (auto &update : event["general game updates"].items())
        {
            if (update.value().is_string())
                game_updates[update.key()] = update.value();
            else
                game_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team a updates"].items())
        {
            if (update.value().is_string())
                team_a_updates[update.key()] = update.value();
            else
                team_a_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team b updates"].items())
        {
            if (update.value().is_string())
                team_b_updates[update.key()] = update.value();
            else
                team_b_updates[update.key()] = update.value().dump();
        }
        
        events.push_back(Event(team_a_name, team_b_name, name, time, game_updates, team_a_updates, team_b_updates, description));
    }
    names_and_events events_and_names{team_a_name, team_b_name, events};

    return events_and_names;
}

std::string Event::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}


std::string Event::extract_value(const std::string& line, const std::string& prefix) {
    return trim(line.substr(prefix.length()));
}