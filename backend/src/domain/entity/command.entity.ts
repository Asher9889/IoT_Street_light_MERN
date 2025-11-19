import { COMMAND_STATUS, LIGHT_STATE, TCommandStatus, TLightState } from "../../constant";
import { IControlNode } from "../../interfaces";
import { logger } from "../../logger";
import { CommandLog } from "../../models";

export interface ICommandInput extends IControlNode {
  type: string;
  cmdId: number;
  nodeId: string;
  gatewayId: string;
  action: string;
  status?: string;
  success?: boolean;
  sentAt: Date;
  ackAt?: Date | null;
  rawPayload?: Record<string, any>;
}


export default class CommandEntity implements ICommandInput {
  type: string;
  cmdId: number;
  nodeId: string;
  gatewayId: string;
  mode: string;
  action: TLightState;
  status: TCommandStatus; 
  success: boolean;
  sentAt: Date;
  ackAt: Date | null = null;
  rawPayload: Record<string, any>;

  constructor(data: ICommandInput) {
    this.type = data.type;
    this.cmdId = data.cmdId;
    this.nodeId = data.nodeId;
    this.gatewayId = data.gatewayId;
    this.action = data.action as TLightState;
    this.status = data.status as TCommandStatus;
    this.success = data.success || false;
    this.sentAt = data.sentAt || new Date();
    this.ackAt = data.ackAt || null;
    this.rawPayload = data.rawPayload || {};
    this.mode = data.mode
  }

  async create(entity: ICommandInput) {
    return CommandLog.create(entity);
  }

  static async markAck(cmdId: number) {
    const result = CommandLog.findOneAndUpdate(
      { cmdId },
      {
        status: COMMAND_STATUS.ACKED,
        success: true,
        ackAt: new Date()
      },
      { new: true }
    ); 
    return result;
  }

  async findByCmdId(cmdId: number) {
    return CommandLog.findOne({ cmdId }).lean();
  }

}
