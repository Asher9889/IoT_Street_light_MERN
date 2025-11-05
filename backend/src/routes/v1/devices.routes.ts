import express, { Request, Response } from "express";
import { relayController, deviceController } from "../../controllers";

const router = express.Router();



// register
router.post("/register", deviceController.register as any)

// update bulb
router.post("/:deviceId/cmd/all", deviceController.updateAllBulb as any)
router.post("/:deviceId/cmd", deviceController.updateSingleBulb as any)

// register gateway
router.post("/gateway", deviceController.registerGateway as any)

// register node
router.post("/node", deviceController.registerNode as any)


export default router;