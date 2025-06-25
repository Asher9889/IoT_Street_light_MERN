import express from "express";
import buldRoutes from "./bulb.routes";


const router = express.Router();

router.use("/bulb", buldRoutes);

export default router;