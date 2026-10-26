import math
import zlib
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import cover, number, stepper
from esphome.const import CONF_ID

DEPENDENCIES = ["stepper"]
AUTO_LOAD = ["number"]
ns = cg.esphome_ns.namespace("stepper_blind")
StepperBlind = ns.class_("StepperBlind", cover.Cover, cg.Component)
EndstopNumber = ns.class_("EndstopNumber", number.Number)
SpeedNumber = ns.class_("SpeedNumber", number.Number)


def valid_speed(value):
    value = stepper.validate_speed(value)
    if not math.isfinite(value) or value < 1:
        raise cv.Invalid("speed muss endlich und mindestens 1 steps/s sein")
    return value


def validate_speed_config(config):
    if "speed" not in config:
        return config
    full = fv.full_config.get()
    path = full.get_path_for_id(config["stepper_id"])[:-1]
    maximum = full.get_config_for_path(path)["max_speed"]
    maximum = valid_speed(maximum)
    speed = config["speed"]
    initial = speed.get("initial_value", maximum)
    if initial > maximum:
        raise cv.Invalid("speed.initial_value darf stepper.max_speed nicht ueberschreiten")
    # Derive the UI and runtime limit from the same motor configuration.
    speed["max_value"] = maximum
    speed["initial_value"] = initial
    return config


FINAL_VALIDATE_SCHEMA = validate_speed_config

def valid_endstop(value):
    value = cv.int_range(min=-90000, max=90000)(value)
    if value == 0:
        raise cv.Invalid("closed_position darf nicht 0 sein")
    return value

CONFIG_SCHEMA = cover.cover_schema(StepperBlind).extend({
    cv.Required("stepper_id"): cv.use_id(stepper.Stepper),
    cv.Optional("closed_position", default=10000): valid_endstop,
    cv.Optional("endstop"): number.number_schema(EndstopNumber),
    cv.Optional("speed"): number.number_schema(
        SpeedNumber, unit_of_measurement="steps/s", icon="mdi:speedometer"
    ).extend({cv.Optional("initial_value"): valid_speed}),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)
    motor = await cg.get_variable(config["stepper_id"])
    cg.add(var.set_stepper(motor))
    cg.add(var.set_closed_position(config["closed_position"]))
    cg.add(var.set_preference_key(zlib.crc32((str(config[CONF_ID]) + ":stepper_blind:v1").encode())))
    if "endstop" in config:
        entity = await number.new_number(config["endstop"], min_value=-90000, max_value=90000, step=1)
        cg.add(entity.set_parent(var))
        cg.add(var.set_endstop_number(entity))
    if "speed" in config:
        speed = config["speed"]
        entity = await number.new_number(speed, min_value=1, max_value=speed["max_value"], step=1)
        cg.add(entity.set_parent(var))
        cg.add(var.set_speed_number(entity))
        cg.add(var.set_speed_config(speed["max_value"], speed["initial_value"]))
