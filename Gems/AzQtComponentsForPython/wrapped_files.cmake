#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(WRAPPED_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtpyside_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_wrapper.cpp
                                                      
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_buttondivider_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_buttonstripe_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_confighelpers_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_confighelpers_groupguard_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_dockbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_dockbarbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_dockbarbutton_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_dockmainwindow_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_docktabbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_docktabwidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_extendedlabel_wrapper.cpp    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_fancydocking_widgetgrab_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_fancydocking_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_fancydockingdropzonewidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_fancydockingghostwidget_wrapper.cpp 
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_filtercriteriabutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_searchtypefilter_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_searchtypeselector_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_filteredsearchwidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_filteredsearchwidget_config_wrapper.cpp

    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/flowlayout_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_globaleventfilter_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_helpbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_interactivewindowgeometrychanger_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_searchlineedit_wrapper.cpp  
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_style_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styledbusylabel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styleddetailstablemodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styleddetailstableview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styleddialog_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styleddockwidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styledlineedit_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styleddoublespinbox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styledspinbox_wrapper.cpp  
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_stylemanager_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_stylesheetcache_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_stylesheetpreprocessor_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tagselector_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_config_titlebar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_config_icon_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_config_title_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebar_config_buttons_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_titlebaroverdrawhandler_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toastnotification_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toastconfiguration_wrapper.cpp  
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbararea_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbuttoncombobox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbuttonlineedit_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbuttonwithwidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_vectoredit_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_windowdecorationwrapper_wrapper.cpp   
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderlistview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderthumbnailview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderthumbnailview_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderthumbnailview_config_thumbnail_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderthumbnailview_config_expandbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_assetfolderthumbnailview_config_childframe_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_breadcrumbs_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_breadcrumbs_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_browseedit_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_card_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_card_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_cardheader_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_cardnotification_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_checkbox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_checkbox_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_colorlabel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_comboboxvalidator_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_combobox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_combobox_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_dialogbuttonbox_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_draganddrop_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_draganddrop_dropindicator_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_draganddrop_dragindicator_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_draganddrop_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_elidinglabel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_eyedropper_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_eyedropper_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_filedialog_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_gradientslider_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_lineedit_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_lineedit_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_menu_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_menu_margins_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_menu_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_azmessagebox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_overlaywidgetbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_overlaywidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_progressbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_progressbar_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_gradient_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_colorset_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_border_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_frame_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_smallicon_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_iconbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_dropdownbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_pushbutton_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_radiobutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_radiobutton_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_scrollbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_scrollbar_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_segmentbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_segmentcontrol_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_border_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_gradientsliderconfig_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_sliderconfig_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_sliderconfig_handleconfig_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_sliderconfig_grooveconfig_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slider_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_sliderint_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_sliderdouble_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_slidercombo_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_sliderdoublecombo_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_spinbox_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_spinbox_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_statusbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_statusbar_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tableview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tableview_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tableviewmodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tableviewitemdelegate_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tabwidgetactiontoolbarcontainer_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tabwidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tabwidget_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tabwidgetactiontoolbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_tabbar_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_text_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_text_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbar_toolbarconfig_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbar_config_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbutton_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_toolbutton_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_treeview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_treeview_config_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_branchdelegate_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styledtreeview_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_styledtreewidget_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_vectoreditelement_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_vectorelement_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_vectorinput_wrapper.cpp
    
    ${CMAKE_CURRENT_BINARY_DIR}/${bindings_library}/azqtcomponents_internal_wrapper.cpp
)
