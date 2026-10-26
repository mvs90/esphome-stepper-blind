import zlib
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover, number, stepper
from esphome.const import CONF_ID

DEPENDENCIES = ["stepper"]
AUTO_LOAD = ["number"]
ns = cg.esphome_ns.namespace("stepper_blind")
StepperBlind = ns.class_("StepperBlind", cover.Cover, cg.Component)
EndstopNumber = ns.class_("EndstopNumber", number.Number)

def valid_endstop(value):
    value = cv.int_range(min=-90000, max=90000)(value)
    if value == 0:
        raise cv.Invalid("closed_position darf nicht 0 sein")
    return value

CONFIG_SCHEMA = cover.cover_schema(StepperBlind).extend({
    cv.Required("stepper_id"): cv.use_id(stepper.Stepper),
    cv.Optional("closed_position", default=10000): valid_endstop,
    cv.Optional("endstop"): number.number_schema(EndstopNumber),
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
