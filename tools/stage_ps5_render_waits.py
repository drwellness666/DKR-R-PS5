#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Observe the actual present worker's waits without changing synchronization."""
def compose(text):
    replacements={
      'ext.workloadQueue->waitForWorkloadId(present.workloadId)':2,
      'ext.presentGraphicsWorker->wait()':4,
      'dkr::runtime::surgical::presentation_pacer().wait(this,presentTimestamp,targetRate)':5,
      'ext.swapChain->wait()':6,
      'ext.swapChain->acquireTexture(acquiredSemaphore.get(), &swapChainIndex)':7,
      'ext.swapChain->present(swapChainIndex, &waitSemaphore, 1)':8,
    }
    for expression,phase in replacements.items():
        if expression not in text:raise ValueError('Missing native render wait: '+expression)
        text=text.replace(expression,'dkrr::render_wait('+str(phase)+',[&]{return '+expression+';})')
    cursor='''cursorCondition.wait(cursorLock, [&]() {
                    return (writeCursor != threadCursor) || !presentThreadRunning;
                });'''
    interpolation='''ext.sharedResources->interpolatedCondition.wait(interpolatedLock, [&]() {
                    return (frameCounters.available > targetIndex) || ((frameCounters.available == targetIndex) && frameCounters.skipped);
                });'''
    for expression,phase in ((cursor,1),(interpolation,3)):
        if text.count(expression)!=1:raise ValueError('Unexpected native condition wait')
        text=text.replace(expression,'dkrr::render_wait('+str(phase)+',[&]{'+expression+'});')
    return '#include "ps5_render_wait.hpp"\n'+text
