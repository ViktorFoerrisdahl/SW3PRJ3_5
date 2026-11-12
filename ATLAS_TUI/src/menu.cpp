#include "menu.h"
#include <iostream>
#include <unistd.h>

menu::menu()
{
}

void menu::showTitle()
{
    std::cout << R"ART(                                                                                 
         ........                                                                                   
      .:=#@@@@@@#=:.                                                                                
    .-#@#=:    .=#@#-.       .:----:.:----------------::..             .:----:.   ..------------:.  
   .+@#: -+:...==.:*@+.     .+@@@@@@=-%@@@@@@@@@@@@@@+:#@#-           .+@@@@@%-.  :@@@@@@@@@@@@%-.  
  .=@*.-+=%@++%%=+-.+@=.   .-@@@%@@@%--****#@@@#****=.:#@@@.         .=@@@#@@@#:  :@@@%#######*-.   
  :##: :#@*+%%+*@%- :#%:  .:%@@%::@@@#:   .:@@@=.     :#@@@.        .-%@@#.-@@@#: :@@@#=========:.  
  :%*:   +@#::#@*   .*@:  .#@@%-. -@@@*:  .:@@@=.     :#@@@.       .:%@@%:. =@@@*.:@@@@@@@@@@@@@%-  
  :##: :#@*+%%+*@#- :#%: .*@@@=.=##@@@@+. .:@@@=.     :#@@@.       .#@@@-.=##@@@@=.-+++++++++*@@%-  
  .=@+.-+=#@*+@%=+=.+@=..*@@@+.  .+@@@@@=..:@@@=.     :#@@@******-.*@@@=.  :*@@@@@-.*********#@@%-  
   .+@*: =+:. :++..*@+. =@@@*.    ..+%@@@:.:@@@=.     .#@@@@@@@@@:+@@@+.    ..+@@@@.#@@@@@@@@@@@%-  
    .-#@#=.    .=#@%-. .===-.       ..-=-. .:-:.       .-=======:.===:.       ..-=-..-=========-:.  
      .:+#@@@%@@%+-.                                                                                
         ........                           
                                                                 
)ART" << std::endl;
    sleep(1);
    // https://www.asciiart.eu/image-to-ascii
}

void menu::showStartingSequence()
{
    std::cout << "MIC's input recieved! NOW starting ATLAS... \n";
    int total = 100;
    for (int i = 0; i <= 100; i++)
    {
        float fraction = static_cast<float>(i) / total;
        int filled = static_cast<int>(fraction * 40);

        std::cout << "\r[";
        for (int i = 0; i < 40; ++i)
        {
            if (i < filled)
                std::cout << '#';
            else
                std::cout << '-';
        }
        std::cout << "] " << static_cast<int>(fraction * 100) << "%" << std::flush;

        if (i == total)
            std::cout << '\n';
        usleep(30000);
    }
    sleep(1);
}

void menu::showMenu()
{
    std::cout << "Welcome to ATLAS! \n";
    sleep(1);
    std::cout << "Before starting ATLAS for the first time - information is needed: \n";
    sleep(2);
}

void menu::showUserOptions()
{
    std::cout << "\n::::::::::::::::::USER MENU:::::::::::::::::: \n";
    std::cout << "--- Press 0 to shutdown ATLAS\n";
    std::cout << "--- Press 1 to update MIC positions\n";
}
