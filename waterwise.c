/* waterwise - A simple home web service to get the bewaterwise.com index.
 *
 * Copyright 2020, Pascal Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "echttp.h"
#include "echttp_static.h"
#include "echttp_json.h"
#include "echttp_xml.h"
#include "houseportalclient.h"

#include "housediscover.h"
#include "houselog.h"
#include "houselog_sensor.h"

static int    WaterWiseIndexDaily = 100;
static int    WaterWiseIndexWeekly = 100;
static int    WaterWiseIndexMonthly = 100;

static int    WaterWisePriority = 10;
static char   WaterWiseState[2] = "u";
static char   WaterWiseError[256] = "";
static time_t WaterWiseUpdate = 0;
static time_t WaterWiseReceived = 0;

static const char *WaterWiseUrl = "http://www.bewaterwise.com/RSS/rsswi.xml";

static const char WaterWiseDailyIndexPath[] =
    ".rss.content.channel.content.item[0].content.description.content";
static const char WaterWiseWeeklyIndexPath[] =
    ".rss.content.channel.content.item[1].content.description.content";
static const char WaterWiseMonthlyIndexPath[] =
    ".rss.content.channel.content.item[2].content.description.content";

static const char *WaterWiseUpdatePath = 
    ".rss.content.channel.content.dateupdated.content";

static const char *WaterWiseIndexType = "Weekly";
static int   *WaterWiseIndex = &WaterWiseIndexWeekly;

static void waterwise_select_index (const char *value) {
    if (!strcasecmp ("daily", value)) {
        WaterWiseIndexType = "Daily";
        WaterWiseIndex = &WaterWiseIndexDaily;
    } else if (!strcasecmp ("weekly", value)) {
        WaterWiseIndexType = "Weekly";
        WaterWiseIndex = &WaterWiseIndexWeekly;
    } else if (!strcasecmp ("monthly", value)) {
        WaterWiseIndexType = "Monthly";
        WaterWiseIndex = &WaterWiseIndexMonthly;
    }
}

static const char *waterwise_status (const char *method, const char *uri,
                                     const char *data, int length) {
    static char buffer[65537];
    static char pool[65537];
    static char host[256];
    static char state[2];

    ParserToken token[1024];

    if (host[0] == 0) gethostname (host, sizeof(host));

    ParserContext context = echttp_json_start (token, 1024, pool, sizeof(pool));

    int root = echttp_json_add_object (context, 0, 0);
    echttp_json_add_string (context, root, "host", host);
    echttp_json_add_string (context, root, "proxy", houseportal_server());
    echttp_json_add_integer (context, root, "timestamp", (long)time(0));
    int top = echttp_json_add_object (context, root, "waterindex");
    int container = echttp_json_add_object (context, top, "status");
    echttp_json_add_string (context, container, "name", "waterwise");
    echttp_json_add_string (context, container, "type", WaterWiseIndexType);
    echttp_json_add_string (context, container, "origin", WaterWiseUrl);
    echttp_json_add_string (context, container, "state", WaterWiseState);
    if (WaterWiseError[0]) {
        echttp_json_add_string (context, container, "error", WaterWiseError);
    } else {
       echttp_json_add_integer (context, container, "index", *WaterWiseIndex);
       echttp_json_add_integer (context, container, "received", (long)WaterWiseReceived);
       echttp_json_add_integer (context, container, "updated", (long)WaterWiseUpdate);
       echttp_json_add_integer (context, container, "priority", (long)WaterWisePriority);
    }

    const char *error = echttp_json_export (context, buffer, sizeof(buffer));
    if (error) {
        echttp_error (500, error);
        return "";
    }
    echttp_content_type_json ();
    return buffer;
}

static const char *waterwise_set (const char *method, const char *uri,
                                  const char *data, int length) {
    const char *value = echttp_parameter_get("index");
    if (value) {
        waterwise_select_index (value);
    }
    return waterwise_status (method, uri, data, length);
}

static void waterwise_response
                (void *origin, int status, char *data, int length) {

    ParserToken tokens[100];
    int  count = 100;

    status = echttp_redirected("GET");
    if (!status) {
        echttp_submit (0, 0, waterwise_response, (void *)0);
        return;
    }

    WaterWiseState[0] = 'e'; // Assume error.
    if (status != 200) {
        snprintf (WaterWiseError, sizeof(WaterWiseError),
                  "HTTP code %d on %s", status, WaterWiseUrl);
        houselog_trace (HOUSE_FAILURE, "HTTP",
                        "ERROR %d on %s", status, WaterWiseUrl);
        return;
    }

    const char *error = echttp_xml_parse (data, tokens, &count);
    if (error) {
        snprintf (WaterWiseError, sizeof(WaterWiseError),
                  "XML syntax error %s", error);
        houselog_trace (HOUSE_FAILURE, "XML", "SYNTAX ERROR %s", error);
        return;
    }
    if (count <= 0) {
        snprintf (WaterWiseError, sizeof(WaterWiseError), "no XML data");
        houselog_trace (HOUSE_FAILURE, "XML", "NO DATA");
        return;
    }

    int olddaily = WaterWiseIndexDaily;
    int oldweekly = WaterWiseIndexWeekly;
    int oldmonthly = WaterWiseIndexMonthly;
    time_t oldupdate = WaterWiseUpdate;

    int index = echttp_json_search (tokens, WaterWiseDailyIndexPath);
    if (index <= 0) {
        snprintf (WaterWiseError, sizeof(WaterWiseError), "no daily index found");
        houselog_trace (HOUSE_FAILURE, "JSON", "NO DAILY INDEX FOUND");
        return;
    }
    WaterWiseIndexDaily = atoi(tokens[index].value.string);

    index = echttp_json_search (tokens, WaterWiseWeeklyIndexPath);
    if (index <= 0) {
        snprintf (WaterWiseError, sizeof(WaterWiseError), "no weekly index found");
        houselog_trace (HOUSE_FAILURE, "JSON", "NO WEEKLY INDEX FOUND");
        return;
    }
    WaterWiseIndexWeekly = atoi(tokens[index].value.string);

    index = echttp_json_search (tokens, WaterWiseMonthlyIndexPath);
    if (index <= 0) {
        snprintf (WaterWiseError, sizeof(WaterWiseError), "no monthly index found");
        houselog_trace (HOUSE_FAILURE, "JSON", "NO MONTHLY INDEX FOUND");
        return;
    }
    WaterWiseIndexMonthly = atoi(tokens[index].value.string);

    WaterWiseState[0] = 'a'; // No error found.
    WaterWiseError[0] = 0;
    WaterWiseReceived = time(0);

    index = echttp_json_search (tokens, WaterWiseUpdatePath);
    if (index <= 0) {
        WaterWiseUpdate = WaterWiseReceived;
    } else {
        struct tm update;
        const char *date = tokens[index].value.string;
        update.tm_mon = atoi(date) - 1;
        update.tm_mday = atoi (strchr(date, '/')+1);
        update.tm_year = atoi (strrchr(date, '/')+1) - 1900;
        update.tm_hour = update.tm_min = update.tm_sec = 0;
        update.tm_isdst = -1;
        WaterWiseUpdate = mktime (&update);
        if (WaterWiseUpdate < 0) WaterWiseUpdate = 0;
        else WaterWiseUpdate += (24 * 3600); // Calculated the day after.
    }

    if ((olddaily == WaterWiseIndexDaily) &&
        (oldweekly == WaterWiseIndexWeekly) &&
        (oldmonthly == WaterWiseIndexMonthly) &&
        (oldupdate == WaterWiseUpdate)) return; // No change.

    houselog_event ("WATERWISE", "INDEX", "NEW",
                    "INDEX %d%% (DAILY) %d%% (WEEKLY) %d%% (MONTHLY)",
                    WaterWiseIndexDaily, WaterWiseIndexWeekly, WaterWiseIndexMonthly);

    struct timeval timestamp;
    timestamp.tv_sec = WaterWiseUpdate;
    timestamp.tv_usec = 0;

    houselog_sensor_numeric
        (&timestamp, "Riverside", "index.daily", WaterWiseIndexDaily, "%");
    houselog_sensor_numeric
        (&timestamp, "Riverside", "index.weekly", WaterWiseIndexWeekly, "%");
    houselog_sensor_numeric
        (&timestamp, "Riverside", "index.monthly", WaterWiseIndexMonthly, "%");
    houselog_sensor_flush ();
}

static void waterwise_background (int fd, int mode) {

    static time_t LastCall = 0;
    time_t now = time(0);

    if (now == LastCall) return;
    LastCall = now;

    houseportal_background (now);
    housediscover (now);
    houselog_background (now);
    houselog_sensor_background (now);

    if (now % 60) return; // Check every minute only.

    if (now < WaterWiseReceived + 12 * 3600) return; // Ask twice a day.

    const char *error = echttp_client ("GET", WaterWiseUrl);
    if (error) {
        snprintf (WaterWiseError, sizeof(WaterWiseError),
                  "cannot connect, %s", error);
        WaterWiseState[0] = 'f';
        return;
    }
    echttp_submit (0, 0, waterwise_response, (void *)0);
}

int main (int argc, const char **argv) {

    // These strange statements are to make sure that fds 0 to 2 are
    // reserved, since this application might output some errors.
    // 3 descriptors are wasted if 0, 1 and 2 are already open. No big deal.
    //
    open ("/dev/null", O_RDONLY);
    dup(open ("/dev/null", O_WRONLY));

    int i;
    const char *value;
    for (i = 1; i < argc; ++i) {
        if (echttp_option_match ("-priority=", argv[i], &value)) {
            WaterWisePriority = atoi(value);
            if (WaterWisePriority < 0) WaterWisePriority = 0;
        } else if (echttp_option_match ("-index=", argv[i], &value)) {
            waterwise_select_index (value);
        }
    }

    echttp_default ("-http-service=dynamic");

    argc = echttp_open (argc, argv);
    if (echttp_dynamic_port()) {
        static const char *path[] = {"waterindex:/waterwise"};
        houseportal_initialize (argc, argv);
        houseportal_declare (echttp_port(4), path, 1);
    }
    housediscover_initialize (argc, argv);
    houselog_initialize ("waterwise", argc, argv);
    houselog_sensor_initialize ("waterwise", argc, argv);

    echttp_route_uri ("/waterwise/set", waterwise_set);
    echttp_route_uri ("/waterwise/status", waterwise_status);
    echttp_static_route ("/", "/usr/local/share/house/public");
    echttp_background (&waterwise_background);
    echttp_loop();
}

