@echo off
REM $Id$
REM @file fix_xml.cmd Adjust the output of the LabVIEW ExportVIStrings() method (which is more like HTML 2.0 than XML!)
REM @author Freddie Akeroyd, STFC ISIS Facility, UK
REM
REM Usage:   fix_xml original_lvexport.txt lvexport.xml
REM
REM NOTE: you need to have "sed" available in your path (binary available from http://gnuwin32.sourceforge.net/packages/sed.htm)
sed -e "s/\([A-Za-z0-9][A-Za-z0-9]*=\)\([A-Za-z0-9][A-Za-z0-9]*\)/\1\"\2\"/g" -e "s/<FONT[^>]*>//g" -e "s/<LF>//g" -e "s/<SAME_AS_LABEL>//g" -e "s/<NO_TITLE[^>]*>//g" -e "s/<<[^>]*>>//g" %1 > %2
