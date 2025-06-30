import express, { Request, Response } from "express";
import { relayController, deviceController } from "../../controllers";

const router = express.Router();



// register
router.post("/register", deviceController.register as any)

// update bulb
router.post("/update-bulb", relayController.updateBulb as any)


export default router;