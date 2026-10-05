APP_NAME = LED_Lamp
APP_ICON = src/icon.png

OUTPUT_DIR = output

SOURCES = $(addprefix src/,\
  main.c \
)

# Place ce dossier dans epsilon/external_apps/ (a cote de sample_c) pour que ce chemin marche
include ../build/external_app.mak
