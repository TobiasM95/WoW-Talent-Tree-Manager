# NOTE:
# This file starts all necessary data update tasks if necessary and should be run daily
from data_management.create_popular_builds import create_popular_builds


def main():
    # Check if presets have been updated, if yes:
    ## run preset_updater
    ## run extract_icons_to_public

    create_popular_builds()


if __name__ == "__main__":
    main()
