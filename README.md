A solver for the pipes puzzle on [puzzle-pipes.com](https://puzzle-pipes.com)

### Features

- Can crawl the website to pull puzzles and solve them automatically
- Can submit puzzles to the hall of fame (as a robot)
- Complete solver, can solve any puzzle that has a solution, or will find a contradiction if there is no solution
- Very experimental puzzle generation

### Usage

This program is tested on linux, but should work on other unix-likes without issue. Windows will require mingw or an alternative build system.
It requires the curl library to be installed for scraping the puzzle site. You can run the make command and it will output `run_solver` and `generate`.
`run_solver` takes command line arguments `./run_solver <size> [-(a)nimate [delay_in_us]] [--(u)ser user_email]`. Size can be a 1 digit number from 0 to 9, which correspond to the non-wrapping puzzle sizes, or 10-19 for wrapping puzzles.
