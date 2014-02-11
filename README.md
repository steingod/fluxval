fluxval
=======

Øystein Godøy, METNO/FOU, 2014-02-11

PURPOSE

Software for validation of radiative fluxes estimated in the context of OSISAF.

Passage and daily estimates are now validated against Bioforsk stations
using the same executable but different command line options.

The compact format of the IPY stations is also supported.

Essentially both DLI and SSI can be validated, but the software still
needs refinement.

TODO
  - Extract handling of cloud mask and observation geometry for passage
    products into separate functions to create a nicer software outline.

DISCLAIMER
This software is currently not well documented, not are all required libraries
easily available (need email currently). 
