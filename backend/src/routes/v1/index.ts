import express from "express";
import buldRoutes from "./bulb.routes";
import devicesRoutes from "./devices.routes";


const router = express.Router();

router.use("/bulb", buldRoutes);
router.use("/devices", devicesRoutes);

export default router;