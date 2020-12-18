
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



QString pfm_fileText = 
  startPage::tr ("Use the browse button to select an input PFM file.  You cannot modify the text in the "
		 "<b>PFM File</b> text window.  The reason for this is that the file must exist in order for the "
		 "program to run.  Note also that the <b>Next</b> button will not work until you select an input "
		 "PFM file.");

QString pfm_fileBrowseText = 
  startPage::tr ("Use this button to select the input PFM file");

QString topoText = 
  startPage::tr ("If you check this box the values for the masked areas will come from the SRTM topo data "
                 "instead of using the fixed value in the <b>Mask value</b> slot.  When this is selected the "
                 "<b>Mask value</b> slot is disabled.<br><br>"
                 "<b>IMPORTANT NOTE: If the NAVO SRTM data files are not available then this check box will "
                 "be disabled.  Check your ABE_DATA environment variable to see if it is pointing to the "
                 "directory that contains the SRTM and land_mask data files.</b>");

QString maskText = 
  startPage::tr ("You may enter the mask value to be stored in PFM cells that have no original input data and "
                 "are marked as land in the 1 second SWBD.");
