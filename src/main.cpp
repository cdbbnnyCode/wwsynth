#include "banks.h"
#include "aaf.h"

#include <stdio.h>

int main(void)
{
  AAFFile f("../data/Banks/");
  if (!f.load("../data/JaiInit.aaf"))
  {
    printf("Failed to load AAF file\n");
    return 1;
  }

  IBNK bank = f.loadBank(54);

  
  
  return 0;
}