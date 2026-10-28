// Точка сборки (composition root): создаёт объекты и связывает их через интерфейсы.
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "config.hpp"
#include "console_app.hpp"
#include "http_client.hpp"
#include "place_service.hpp"
#include "providers.hpp"
#include "retrying_http_client.hpp"
#include "text_report_renderer.hpp"

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    ApiConfig config = ApiConfig::load(".env");
    if (!config.hasAllKeys()) std::cerr << "Предупреждение: не заданы API-ключи (.env или переменные окружения)\n";

    HttpClient rawHttp;
    RetryingHttpClient http(rawHttp, 2);

    GraphHopperGeocoder graphHopper(http, config.graphhopperUrl(), config.graphhopperKey());
    NominatimGeocoder nominatim(http, config.nominatimUrl());
    FallbackGeocoder geocoder(graphHopper, nominatim);
    OpenWeatherProvider weather(http, config.openweatherUrl(), config.openweatherKey());
    OpenTripMapCatalog catalog(http, config.opentripmapUrl(), config.opentripmapKey());

    PlaceService service(geocoder, weather, catalog);
    TextReportRenderer renderer;
    ConsoleApp app(service, renderer, std::cin, std::cout);
    app.run();
    return 0;
}