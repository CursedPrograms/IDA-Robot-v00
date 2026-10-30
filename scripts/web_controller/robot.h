// IDA's controller window: IDA has no WiFi, so this opens NORA's page, which drives her over the IR link
#pragma once

#define ROBOT_NAME L"IDA"
#define ROBOT_NAME_U8 "IDA"
#define ROBOT_FLEET_NAME "NORA"
#define ROBOT_HOST L"192.168.4.1"
#define ROBOT_PORT 5002
#define ROBOT_PATH L"/"
#define ROBOT_WIDTH 1200
#define ROBOT_HEIGHT 820

// colour_scheme.xml, for the page shown before there's a robot to show
#define ROBOT_BACKGROUND "#090B10"
#define ROBOT_PANEL "#11141D"
#define ROBOT_BORDER "#22293A"
#define ROBOT_TEXT "#F2EDE9"
#define ROBOT_ACCENT "#D6935A"
