// SPDX-License-Identifier: GPL-3.0-or-later
DetailCard(width,{22.0F,22.0F},[&](float inner) {
    const auto update=dkrr::updates::view();
    using UpdateState=dkrr::updates::State;
    DetailHeading("PS5 UPDATES",inner,kSetWarm);
    bool automatic=dkrr::updates::automatic_check();
    if(ImGui::Checkbox("Check for updates at launch",&automatic)&&!dkrr::updates::set_automatic_check(automatic))
        DetailText("The update preference could not be saved.",inner);
    PaddockGap(12.0F);
    DetailText(update.message[0]?update.message:"Updates are checked once each launch.",inner);
    if(update.state==UpdateState::Downloading&&update.bytes)
        ImGui::ProgressBar(float(double(update.received)/double(update.bytes)),{inner,0.0F});
    const bool waiting=update.state==UpdateState::Checking||update.state==UpdateState::Downloading||
        update.state==UpdateState::HandingOff||update.state==UpdateState::Closing;
    ImGui::BeginDisabled(waiting);
    if(DetailRaceButton("CHECK FOR UPDATES",inner))dkrr::updates::check();
    ImGui::EndDisabled();
    if(update.state==UpdateState::Available){
        PaddockGap(12.0F);
        if(DetailRaceButton("INSTALL UPDATE AND CLOSE DKR-R",inner))dkrr::updates::install();
        DetailText("Downloads on this PS5, then closes the game before installation. Progress, settings and texture packs are kept. Start DKR-R again after the completion notification.",inner,kSetMuted);
    }
    if(update.state==UpdateState::Downloading){PaddockGap(12.0F);if(DetailRaceButton("CANCEL DOWNLOAD",inner))dkrr::updates::cancel();}
});
PaddockGap(16.0F);
