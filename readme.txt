Program features:
- The first process in the simulation will spawn Smaug, and will spawn (over time) each of the cows, sheep, hunters and thieves.
- Sheep arrives (new sheep processes are created) randomly. 
      - The length of the interval between the arrivals of successive sheep is drawn from a uniform distribution. 
      - The length of interval n is the length of time between the arrival of sheep n and the arrival of sheep n+1.
      - The distribution of possible interval lengths is uniform between 0 second and maximumSheepInterval seconds (no arrival interval outside this range).
- Cows arrive (new cow processes are created) randomly. 
      - The length of the interval between the arrivals of successive cows is drawn from a uniform distribution. 
      - The distribution of possible interval lengths is uniform between 0 second and maximumCowInterval seconds (no arrival interval outside this range).
- Similarly, treasure hunters arrive at random intervals drawn from a uniform distribution. 
      - The distribution of possible interval lengths is uniform between 0 second and maximumHunterInterval seconds (no arrival interval outside this range).
- Similarly, thieves arrive at random intervals drawn from a uniform distribution. 
      - The distribution of possible interval lengths is uniform between 0 second and maximumThiefInterval or maximumHunterInterval seconds respectively (no arrival interval outside this range).
- Smaug’s treasure initially contains 400 jewels
- The probability that a particular treasure hunter or thief will not be defeated is winProb.
- Program will request the values of maximumSheepInterval, maximumCowInterval, maximumHunterInterval, maximumThiefInterval and winProb from the user and use those values to initialize your simulation.

The simulation terminates when any one of the following conditions is met:
 - Smaug has eaten 14 sheep and 14 cows
 - Smaug has defeated 12 treasure hunters or 15 thieves
 - Smaug has no treasure
 - Smaug has 800 jewels.
