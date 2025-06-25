import express, { Request, Response } from "express";
import { relayController } from "../../controllers";

const router = express.Router();



// active bulb : ON or OFF
router.get("/latest-command", relayController.getLatestCommand as any)

// update bulb
router.post("/update-bulb", relayController.updateBulb as any)


export default router;