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
        usleep(5000);
    }
}

void menu::showMenu()
{
    std::cout << "press 1 and enter for plot \n";
}
