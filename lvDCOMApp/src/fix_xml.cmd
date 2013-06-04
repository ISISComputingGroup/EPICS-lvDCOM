@echo off
REM $Id$
REM @author Freddie Akeroyd, STFC ISIS Facility, UK
REM fix the XML output by the LabVIEW ExportVIStrings() method (which is more like HTML 2.0 than XML!)
REM
REM Usage:   fix_xml original_lvexport.txt lvexport.xml
REM
sed -e "s/\([A-Za-z0-9][A-Za-z0-9]*=\)\([A-Za-z0-9][A-Za-z0-9]*\)/\1\"\2\"/g" -e "s/<FONT[^>]*>//g" -e "s/<LF>//g" -e "s/<SAME_AS_LABEL>//g" -e "s/<NO_TITLE[^>]*>//g" -e "s/<<[^>]*>>//g" %1 > %2