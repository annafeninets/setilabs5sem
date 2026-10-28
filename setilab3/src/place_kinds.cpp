#include "place_kinds.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace {

constexpr std::size_t kMaxShownKinds = 2;

const std::map<std::string, std::string>& specificKinds() {
    static const std::map<std::string, std::string> m = {
        {"bus_stops", "автобусная остановка"},   {"bus_stations", "автовокзал"},
        {"tram_stops", "трамвайная остановка"},  {"railway_stations", "ж/д станция"},
        {"subway", "метро"},                      {"bicycle_rental", "прокат велосипедов"},
        {"car_rental", "прокат авто"},            {"car_sharing", "каршеринг"},
        {"fuels", "АЗС"},                         {"parkings", "парковка"},
        {"airports", "аэропорт"},                 {"ports", "порт"},
        {"museums", "музей"},                     {"art_galleries", "картинная галерея"},
        {"theatres_and_entertainments", "театр / развлечения"}, {"cinemas", "кинотеатр"},
        {"libraries", "библиотека"},              {"zoos", "зоопарк"},
        {"aquariums", "аквариум"},                {"planetariums", "планетарий"},
        {"circuses", "цирк"},                     {"concert_halls", "концертный зал"},
        {"monuments_and_memorials", "памятник / мемориал"}, {"monuments", "памятник"},
        {"memorials", "мемориал"},                {"historic_architecture", "историческая архитектура"},
        {"fortifications", "крепость / укрепление"}, {"castles", "замок"},
        {"palaces", "дворец"},                    {"towers", "башня"},
        {"bridges", "мост"},                      {"skyscrapers", "небоскрёб"},
        {"fountains", "фонтан"},                  {"sculptures", "скульптура"},
        {"gardens_and_parks", "сад / парк"},
        {"parks", "парк"},                        {"playgrounds", "детская площадка"},
        {"squares", "площадь"},                   {"pedestrian_streets", "пешеходная улица"},
        {"cemeteries", "кладбище"},               {"burial_places", "место захоронения"},
        {"churches", "церковь"},                  {"cathedrals", "собор"},
        {"monasteries", "монастырь"},             {"mosques", "мечеть"},
        {"synagogues", "синагога"},               {"other_temples", "храм"},
        {"view_points", "смотровая площадка"},    {"beaches", "пляж"},
        {"water", "водоём"},                      {"geological_formations", "геологический объект"},
        {"nature_reserves", "заповедник"},        {"amusement_parks", "парк аттракционов"},
        {"water_parks", "аквапарк"},              {"attractions", "аттракцион"},
        {"stadiums", "стадион"},                  {"swimming_pools", "бассейн"},
        {"sports_centres", "спортивный центр"},   {"skiing", "горнолыжный курорт"},
        {"restaurants", "ресторан"},              {"cafes", "кафе"},
        {"fast_food", "фастфуд"},                 {"bars", "бар"},
        {"pubs", "паб"},                          {"biergartens", "пивной сад"},
        {"food_courts", "фудкорт"},               {"bakeries", "пекарня"},
        {"supermarkets", "супермаркет"},          {"malls", "торговый центр"},
        {"marketplaces", "рынок"},                {"convenience", "магазин у дома"},
        {"banks", "банк"},                        {"atms", "банкомат"},
        {"pharmacies", "аптека"},                 {"hospitals", "больница"},
        {"hotels", "отель"},                      {"hostels", "хостел"},
        {"apartments", "апартаменты"},            {"guest_houses", "гостевой дом"},
        {"campsites", "кемпинг"},                 {"tourist_information", "туристический центр"},
        {"toilets", "туалет"},                    {"drinking_water", "питьевая вода"},
        {"picnic_sites", "место для пикника"},    {"post_offices", "почта"},
    };
    return m;
}

const std::map<std::string, std::string>& broadKinds() {
    static const std::map<std::string, std::string> m = {
        {"transport", "транспорт"},       {"cultural", "культура"},
        {"urban_environment", "городская среда"},
        {"historic", "историческое место"}, {"architecture", "архитектура"},
        {"natural", "природа"},           {"religion", "религия"},
        {"sport", "спорт"},               {"amusements", "развлечения"},
        {"foods", "еда"},                 {"shops", "магазин"},
        {"banks", "банк"},                {"accomodations", "жильё"},
        {"tourist_facilities", "туристическая инфраструктура"},
        {"adult", "для взрослых"},        {"industrial_facilities", "промышленный объект"},
    };
    return m;
}

const std::map<std::string, std::string>& osmFeatures() {
    static const std::map<std::string, std::string> m = {
        {"highway=bus_stop", "автобусная остановка"}, {"highway=residential", "улица"},
        {"highway=living_street", "улица"},           {"highway=service", "проезд"},
        {"highway=unclassified", "улица"},            {"highway=tertiary", "улица"},
        {"highway=secondary", "улица"},               {"highway=primary", "магистраль"},
        {"highway=trunk", "магистраль"},              {"highway=motorway", "автомагистраль"},
        {"highway=pedestrian", "пешеходная улица"},  {"highway=footway", "пешеходная дорожка"},
        {"highway=path", "тропа"},                    {"highway=track", "грунтовая дорога"},
        {"highway=steps", "лестница"},                {"highway=platform", "остановочная платформа"},
        {"highway=crossing", "пешеходный переход"},   {"highway=traffic_signals", "светофор"},
        {"highway=cycleway", "велодорожка"},
        {"public_transport=platform", "остановочная платформа"}, {"public_transport=stop_position", "остановка"},
        {"public_transport=station", "станция"},
        {"railway=station", "ж/д станция"},           {"railway=halt", "ж/д платформа"},
        {"railway=tram_stop", "трамвайная остановка"}, {"railway=subway_entrance", "вход в метро"},
        {"railway=rail", "железная дорога"},
        {"amenity=cafe", "кафе"},                     {"amenity=restaurant", "ресторан"},
        {"amenity=fast_food", "фастфуд"},             {"amenity=bar", "бар"},
        {"amenity=pub", "паб"},                       {"amenity=bank", "банк"},
        {"amenity=atm", "банкомат"},                  {"amenity=pharmacy", "аптека"},
        {"amenity=hospital", "больница"},             {"amenity=clinic", "клиника"},
        {"amenity=school", "школа"},                  {"amenity=kindergarten", "детский сад"},
        {"amenity=university", "университет"},        {"amenity=college", "колледж"},
        {"amenity=parking", "парковка"},              {"amenity=fuel", "АЗС"},
        {"amenity=cinema", "кинотеатр"},              {"amenity=theatre", "театр"},
        {"amenity=library", "библиотека"},            {"amenity=place_of_worship", "храм"},
        {"amenity=marketplace", "рынок"},             {"amenity=police", "полиция"},
        {"amenity=post_office", "почта"},             {"amenity=toilets", "туалет"},
        {"building=yes", "здание"},                   {"building=apartments", "жилой дом"},
        {"building=house", "дом"},                    {"building=residential", "жилой дом"},
        {"building=commercial", "коммерческое здание"}, {"building=church", "церковь"},
        {"shop=supermarket", "супермаркет"},          {"shop=convenience", "магазин у дома"},
        {"shop=mall", "торговый центр"},
        {"tourism=hotel", "отель"},                   {"tourism=hostel", "хостел"},
        {"tourism=museum", "музей"},                  {"tourism=attraction", "достопримечательность"},
        {"tourism=viewpoint", "смотровая площадка"},  {"tourism=gallery", "галерея"},
        {"tourism=zoo", "зоопарк"},
        {"leisure=park", "парк"},                     {"leisure=playground", "детская площадка"},
        {"leisure=stadium", "стадион"},               {"leisure=garden", "сад"},
        {"leisure=sports_centre", "спортивный центр"}, {"leisure=pitch", "спортивная площадка"},
        {"place=country", "страна"},                  {"place=state", "регион"},
        {"place=region", "регион"},                   {"place=county", "район"},
        {"place=city", "город"},                      {"place=town", "город"},
        {"place=village", "село / посёлок"},          {"place=hamlet", "деревня"},
        {"place=suburb", "район города"},             {"place=neighbourhood", "микрорайон"},
        {"place=quarter", "квартал"},                 {"place=square", "площадь"},
        {"place=house", "дом"},                       {"place=island", "остров"},
        {"natural=water", "водоём"},                  {"natural=beach", "пляж"},
        {"natural=peak", "вершина"},                  {"natural=wood", "лес"},
        {"historic=monument", "памятник"},            {"historic=memorial", "мемориал"},
        {"historic=castle", "замок"},                 {"historic=ruins", "руины"},
        {"boundary=administrative", "административная граница"},
    };
    return m;
}

const std::map<std::string, std::string>& osmKeyFallbacks() {
    static const std::map<std::string, std::string> m = {
        {"highway", "дорога"},      {"building", "здание"},       {"amenity", "заведение / сервис"},
        {"shop", "магазин"},        {"tourism", "туристический объект"}, {"leisure", "место отдыха"},
        {"place", "населённый пункт"}, {"natural", "природный объект"}, {"historic", "историческое место"},
        {"railway", "железная дорога"}, {"public_transport", "общественный транспорт"},
        {"boundary", "граница"},    {"landuse", "территория"},    {"office", "офис"},
    };
    return m;
}

const std::set<std::string>& ignoredKinds() {
    static const std::set<std::string> s = {"interesting_places", "other", "unclassified_objects"};
    return s;
}

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= s.size()) {
        std::size_t end = s.find(',', start);
        if (end == std::string::npos) end = s.size();
        if (end > start) parts.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

std::string humanize(std::string kind) {
    std::replace(kind.begin(), kind.end(), '_', ' ');
    return kind;
}

}

std::string describePlaceKinds(const std::string& kinds) {
    std::vector<std::string> specific, broad, unknown;
    for (const auto& kind : split(kinds)) {
        if (ignoredKinds().count(kind)) continue;
        if (auto it = specificKinds().find(kind); it != specificKinds().end()) specific.push_back(it->second);
        else if (auto bt = broadKinds().find(kind); bt != broadKinds().end()) broad.push_back(bt->second);
        else unknown.push_back(humanize(kind));
    }
    const std::vector<std::string>& chosen = !specific.empty() ? specific : (!broad.empty() ? broad : unknown);
    std::string result;
    for (std::size_t i = 0; i < chosen.size() && i < kMaxShownKinds; ++i) {
        if (i) result += ", ";
        result += chosen[i];
    }
    return result;
}

std::string describeOsmFeature(const std::string& key, const std::string& value) {
    if (key.empty()) return "";
    if (auto it = osmFeatures().find(key + "=" + value); it != osmFeatures().end()) return it->second;
    if (auto it = osmKeyFallbacks().find(key); it != osmKeyFallbacks().end()) return it->second;
    return humanize(value);
}