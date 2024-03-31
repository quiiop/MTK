#!/usr/bin/python
import os

procedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'nand0'],
                    ['fastboot', 'flash', 'nand0', 'mbl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'vad_sdk_demo.bin']]
}

userprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'flash', 'system_a', 'system.ubi'] ]
}

bootprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'nand0'],
                    ['fastboot', 'flash', 'nand0', 'mbl2.img'],
                    ['fastboot', 'flash', 'bl2', 'mbl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'system.ubi'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ubi'] ]
}

testprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'bl1'],
                    ['fastboot', 'erase', 'bl2'],
                    ['fastboot', 'erase', 'misc'],
                    ['fastboot', 'erase', 'tee_a'],
                    ['fastboot', 'erase', 'boot_a'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'erase', 'userdata'],
                    ['fastboot', 'flash', 'bl1', 'mbl2.img'],
                    ['fastboot', 'flash', 'bl2', 'mbl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'system.ubi'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ubi'] ]
}

# return procedure list
def getFlashProc(product):
    try:
        ret = procedures[product.upper()]
        return ret
    except Exception, e:
        return None


def getFlashUserProc(product):
    try:
        ret = userprocedures[product.upper()]
        return ret
    except Exception, e:
        return None

def getFlashBootProc(product):
    try:
        ret = bootprocedures[product.upper()]
        return ret
    except Exception, e:
        return None

def getFlashTestProc(product):
    try:
        ret = testprocedures[product.upper()]
        return ret
    except Exception, e:
        return None
