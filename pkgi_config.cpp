extern "C" {
#include "pkgi_config.h"
#include "pkgi.h"
}

static char* skipnonws(char* text, char* end)
{
    while (text < end && *text != ' ' && *text != '\n' && *text != '\r')
    {
        text++;
    }
    return text;
}

static char* skipws(char* text, char* end)
{
    while (text < end && (*text == ' ' || *text == '\n' || *text == '\r'))
    {
        text++;
    }
    return text;
}

static DbSort parse_sort(const char* value, DbSort sort)
{
    if (pkgi_stricmp(value, "title") == 0)
    {
        return SortByTitle;
    }
    else if (pkgi_stricmp(value, "region") == 0)
    {
        return SortByRegion;
    }
    else if (pkgi_stricmp(value, "name") == 0)
    {
        return SortByName;
    }
    else if (pkgi_stricmp(value, "size") == 0)
    {
        return SortBySize;
    }
    else
    {
        return sort;
    }
}

static DbSortOrder parse_order(const char* value, DbSortOrder order)
{
    if (pkgi_stricmp(value, "asc") == 0)
    {
        return SortAscending;
    }
    else if (pkgi_stricmp(value, "desc") == 0)
    {
        return SortDescending;
    }
    else
    {
        return order;
    }
}

static DbFilter parse_filter(char* value, uint32_t filter)
{
    uint32_t result = 0;

    char* start = value;
    for (;;)
    {
        char ch = *value;
        if (ch == 0 || ch == ',')
        {
            *value = 0;
            if (pkgi_stricmp(start, "ASA") == 0)
            {
                result |= DbFilterRegionASA;
            }
            else if (pkgi_stricmp(start, "EUR") == 0)
            {
                result |= DbFilterRegionEUR;
            }
            else if (pkgi_stricmp(start, "JPN") == 0)
            {
                result |= DbFilterRegionJPN;
            }
            else if (pkgi_stricmp(start, "USA") == 0)
            {
                result |= DbFilterRegionUSA;
            }
            else
            {
                return static_cast<DbFilter>(filter);
            }
            if (ch == 0)
            {
                break;
            }
            value++;
            start = value;
        }
        else
        {
            value++;
        }
    }

    return static_cast<DbFilter>(result);
}

void pkgi_load_config(
        Config* config,
        char* games_url,
        uint32_t games_len,
        char* updates_url,
        uint32_t updates_len,
        char* dlcs_url,
        uint32_t dlcs_len)
{
    dlcs_url[0] = updates_url[0] = games_url[0] = 0;
    config->sort = SortByName;
    config->order = SortAscending;
    config->filter = DbFilterAll;
    config->no_version_check = 0;

    char data[4096];
    char path[256];
    pkgi_snprintf(
            path, sizeof(path), "%s/config.txt", pkgi_get_config_folder());
    LOG("config location: %s", path);

    int loaded = pkgi_load(path, data, sizeof(data) - 1);
    if (loaded > 0)
    {
        data[loaded] = '\n';

        LOG("config.txt loaded, parsing");
        char* text = data;
        char* end = data + loaded + 1;

        if (loaded > 3 && (uint8_t)text[0] == 0xef &&
            (uint8_t)text[1] == 0xbb && (uint8_t)text[2] == 0xbf)
        {
            text += 3;
        }

        while (text < end)
        {
            char* key = text;

            text = skipnonws(text, end);
            if (text == end)
                break;

            *text++ = 0;

            text = skipws(text, end);
            if (text == end)
                break;

            char* value = text;

            text = skipnonws(text, end);
            if (text == end)
                break;

            *text++ = 0;

            text = skipws(text, end);

            if (pkgi_stricmp(key, "url") == 0 ||
                pkgi_stricmp(key, "url_games") == 0)
            {
                pkgi_strncpy(games_url, games_len, value);
            }
            else if (pkgi_stricmp(key, "url_updates") == 0)
            {
                pkgi_strncpy(updates_url, updates_len, value);
            }
            else if (pkgi_stricmp(key, "url_dlcs") == 0)
            {
                pkgi_strncpy(dlcs_url, dlcs_len, value);
            }
            else if (pkgi_stricmp(key, "sort") == 0)
            {
                config->sort = parse_sort(value, SortByName);
            }
            else if (pkgi_stricmp(key, "order") == 0)
            {
                config->order = parse_order(value, SortAscending);
            }
            else if (pkgi_stricmp(key, "filter") == 0)
            {
                config->filter = parse_filter(value, DbFilterAll);
            }
            else if (pkgi_stricmp(key, "no_version_check") == 0)
            {
                config->no_version_check = 1;
            }
        }
    }
    else
    {
        LOG("config.txt cannot be loaded, using default values");
    }
}

static const char* sort_str(DbSort sort)
{
    switch (sort)
    {
    case SortByTitle:
        return "title";
    case SortByRegion:
        return "region";
    case SortByName:
        return "name";
    case SortBySize:
        return "size";
    default:
        return "";
    }
}

static const char* order_str(DbSortOrder order)
{
    switch (order)
    {
    case SortAscending:
        return "asc";
    case SortDescending:
        return "desc";
    default:
        return "";
    }
}

void pkgi_save_config(
        const Config* config,
        const char* games_url,
        const char* updates_url,
        const char* dlcs_url)
{
    char data[4096];
    int len = 0;
    if (games_url && games_url[0] != 0)
        len += pkgi_snprintf(
                data + len, sizeof(data) - len, "url_games %s\n", games_url);
    if (updates_url && updates_url[0] != 0)
        len += pkgi_snprintf(
                data + len,
                sizeof(data) - len,
                "url_updates %s\n",
                updates_url);
    if (dlcs_url && dlcs_url[0] != 0)
        len += pkgi_snprintf(
                data + len, sizeof(data) - len, "url_dlcs %s\n", dlcs_url);
    len += pkgi_snprintf(
            data + len,
            sizeof(data) - len,
            "sort %s\n",
            sort_str(config->sort));
    len += pkgi_snprintf(
            data + len,
            sizeof(data) - len,
            "order %s\n",
            order_str(config->order));
    len += pkgi_snprintf(data + len, sizeof(data) - len, "filter ");
    const char* sep = "";
    if (config->filter & DbFilterRegionASA)
    {
        len += pkgi_snprintf(data + len, sizeof(data) - len, "%sASA", sep);
        sep = ",";
    }
    if (config->filter & DbFilterRegionEUR)
    {
        len += pkgi_snprintf(data + len, sizeof(data) - len, "%sEUR", sep);
        sep = ",";
    }
    if (config->filter & DbFilterRegionJPN)
    {
        len += pkgi_snprintf(data + len, sizeof(data) - len, "%sJPN", sep);
        sep = ",";
    }
    if (config->filter & DbFilterRegionUSA)
    {
        len += pkgi_snprintf(data + len, sizeof(data) - len, "%sUSA", sep);
        sep = ",";
    }
    len += pkgi_snprintf(data + len, sizeof(data) - len, "\n");

    if (config->no_version_check)
    {
        len += pkgi_snprintf(
                data + len, sizeof(data) - len, "no_version_check 1\n");
    }

    char path[256];
    pkgi_snprintf(
            path, sizeof(path), "%s/config.txt", pkgi_get_config_folder());

    if (pkgi_save(path, data, len))
    {
        LOG("saved config.txt");
    }
    else
    {
        LOG("cannot save config.txt");
    }
}
