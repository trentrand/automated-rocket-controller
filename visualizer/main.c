#include "errno.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "raylib.h"
#include "rlgl.h"

typedef struct GeocoordinateList {
  size_t length;
  Vector3 records[];
} GeocoordinateList;

void parseInputData(char filePath[], GeocoordinateList **parsedInputData) {
  FILE *inputData = fopen(filePath, "r");
  if (inputData == NULL) {
    fclose(inputData);
    fprintf(stderr, "Cannot find GPS input data file at path %s\n", filePath);
    exit(EXIT_FAILURE);
  }

  char *inputLine = NULL;
  size_t lineLength = 0;
  ssize_t numCharactersRead;

  size_t geocoordinateListLength = 0;

  if ((numCharactersRead = getline(&inputLine, &lineLength, inputData)) != -1) {
    if (sscanf(inputLine, "%zu", &geocoordinateListLength) == 1) {
      printf("Number of geocoordinate records: %zu\n", geocoordinateListLength);
    } else if (errno != 0) {
      perror("sscanf");
    } else {
      fprintf(stderr, "First line must contain number of Geocoordinate records.\n");
    }
  }

  // Initiate GeocoordinateList with flexible array member
  *parsedInputData = (GeocoordinateList *) malloc(sizeof(GeocoordinateList) +
    (sizeof(Vector3) * geocoordinateListLength));

  if (parsedInputData == NULL) {
    fprintf(stderr, "Failed to allocate memory for Geocoordinate List.\n");
    exit(EXIT_FAILURE);
  }
  (*parsedInputData)->length = geocoordinateListLength;

  size_t currentRecordIndex = 0;

  while ((numCharactersRead = getline(&inputLine, &lineLength, inputData)) != -1) {
    if (currentRecordIndex >= (*parsedInputData)->length) {
      break;
    }

    Vector3 record = {0};
    sscanf(inputLine, "V(%f, %f, %f)", &(record.x), &(record.y), &(record.z));
    (*parsedInputData)->records[currentRecordIndex] = record;

    printf("%zu: Vector3(%f, %f, %f)\n", currentRecordIndex, record.x, record.y, record.z);

    currentRecordIndex++;
  }

  fclose(inputData);
}

int main(int argc, char *argv[argc+1]) {
  if (argc > 2) {
    printf("Too many arguments supplied.\n");
  } else if (argc == 1) {
    printf("One argument expected. Provide path to gps input data.\n");
    exit(EXIT_FAILURE);
  }

  char inputFilePath[strlen(argv[1])];
  strcpy(inputFilePath, argv[1]);

  GeocoordinateList *inputData;
  parseInputData(inputFilePath, &inputData);
  printf("Number of records parsed: %zu \n", inputData->length);

  const int screenWidth = 1920;
  const int screenHeight = 1080;

  InitWindow(screenWidth, screenHeight, "Automated Rocket Control Visualizer");

  const float visualizerScale = 0.00001f; // 0.00001 units in visualizer == 1 meter in simulator

  const float earthRadius = 600000.0f;
  const float earthAtmosphereAltitude = 70000.0f;

  const float moonRadius = 200000.0f;
  const float moonOrbitRadius = 12000000.0f;
  const float rotationSpeed = 0.1f;

  float moonRotation = 0.0f;
  float moonOrbitRotation = 0.0f;

  int vehiclePositionIndex = 0;
  Model vehicleModel = LoadModelFromMesh(GenMeshPoly(3, 10000));

  Camera camera = { 0 };
  const float cameraPosition = 100.0f;
  camera.position = (Vector3){ cameraPosition, cameraPosition, cameraPosition };
  camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
  camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.type = CAMERA_PERSPECTIVE;;

  SetCameraMode(camera, CAMERA_THIRD_PERSON);

  SetTargetFPS(60);

  while (!WindowShouldClose()) {

    moonRotation += (5.0f * rotationSpeed);
    moonOrbitRotation += (365 / 360.0f * (5.0f * rotationSpeed) * rotationSpeed);

    if (vehiclePositionIndex < inputData->length - 1) {
      vehiclePositionIndex += 1;
    } else {
      vehiclePositionIndex = 0;
    }

    UpdateCamera(&camera);

    BeginDrawing();
    {
      ClearBackground((Color){ 2, 7, 9, 200 });

      BeginBlendMode(BLEND_ALPHA);

      BeginMode3D(camera);
      {
        rlScalef(visualizerScale, visualizerScale, visualizerScale);

        DrawCircle3D((Vector3){ 0.0f, 0.0f, 0.0f }, moonOrbitRadius, (Vector3){ 1, 0, 0 }, 90.0f, Fade(WHITE, 0.15f));

        rlPushMatrix();
          rlRotatef(moonOrbitRotation, 0.0f, 1.0f, 0.0f);
          rlTranslatef(moonOrbitRadius, 0.0f, 0.0f);
          rlRotatef(-moonOrbitRotation, 0.0f, 1.0f, 0.0f);

          rlPushMatrix();
            rlRotatef(moonRotation, 0.25, 1.0, 0.0);
            DrawSphere((Vector3){ 0.0f, 0.0f, 0.0f }, moonRadius, LIGHTGRAY);
          rlPopMatrix();
        rlPopMatrix();

        DrawSphere((Vector3){ 0.0f, 0.0f, 0.0f }, earthRadius, (Color){ 93, 161, 224, 255 });

        DrawModel(vehicleModel, inputData->records[vehiclePositionIndex], 1.0f, WHITE);

        for (size_t recordIndex = 0; recordIndex < inputData->length; recordIndex++) {
          if (recordIndex == inputData->length - 1) { continue; }
          DrawLine3D(inputData->records[recordIndex], inputData->records[(recordIndex + 1) % inputData->length], Fade(WHITE, 0.25f));
        }

        DrawSphere((Vector3){ 0.0f, 0.0f, 0.0f }, earthRadius + earthAtmosphereAltitude, Fade(WHITE, 0.066f));
      }
      EndMode3D();

      EndBlendMode();
    }
    EndDrawing();
  }

  CloseWindow();

  free(inputData);
  UnloadModel(vehicleModel);

  return EXIT_SUCCESS;
}
